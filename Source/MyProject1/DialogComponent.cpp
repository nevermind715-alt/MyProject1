#include "DialogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "QuestComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MyProject1GameInstance.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"
#include "WarpPortal.h"
#include "MyProject1HUD.h"
#include "RpgCharacterInterface.h"
#include "QuestNPCBase.h"
#include "GameplayActionLibrary.h"
#include "EventDistributorComponent.h"
#include "WBP_ChoiceMenu.h"
#include "WBP_DialogAdvance.h"
#include "WBP_FadeNarration.h"
#include "Blueprint/UserWidget.h"

UDialogComponent::UDialogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogComponent::StartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC)
{
	// Blueprint互換のためvoidのまま。実処理と成否判定はTryStartDialogに委譲する
	TryStartDialog(RowName, DialogTable, InNPC);
}

bool UDialogComponent::TryStartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC)
{
	if (!DialogTable) return false;


	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
	{
		RpgInterface->CancelTarget();
	}

	CurrentTable = DialogTable;
	CurrentNPC = InNPC;

	// 暗転ナレーション状態が残ったまま新しい会話が始まることのないよう、念のためここでリセットする
	// （通常フローではBeginFadeNarration/AdvanceFadeNarrationLineが正しく片付けるため、ここは保険）
	bIsFadeNarrationPending = false;
	bIsShowingFadeNarration = false;

	FDialogData* Data = DialogTable->FindRow<FDialogData>(RowName, TEXT("DialogContext"));
	if (!Data)
	{
		// 行名の設定漏れ・タイプミス。ここでfalseを返し、呼び出し側が入力ロックしないようにすることで
		// 「会話UIは出ないのに操作だけロックされる」ソフトロックを防ぐ
		UE_LOG(LogTemp, Warning, TEXT("[Dialog] 行 '%s' が %s に見つかりません。会話を開始できませんでした。"),
			*RowName.ToString(), *GetNameSafe(DialogTable));
		return false;
	}

	CurrentDialogData = *Data;

	// このセリフにループ用AnimEventが設定されていれば、セリフの表示と同時に再生を始める（同じIDが再生中なら継続）。
	// ただし、このセリフ自身が暗転ナレーションを起動する行（選択肢なしのShowTextDuringFade）の場合は、今は画面が見えているため始めない。
	// 画面が真っ暗になって見えなくなった時（BeginFadeNarration）に始める
	const bool bRowStartsNarration = CurrentDialogData.Choices.Num() == 0
		&& CurrentDialogData.ActionType == EDialogActionType::ShowTextDuringFade;
	if (!bRowStartsNarration)
	{
		StartRowLoopAnimEvent(CurrentDialogData.LoopAnimEventID);
	}

	FString RawText = CurrentDialogData.DialogText.ToString();
	CurrentDialogLines.Empty();

	RawText = RawText.Replace(TEXT("\r"), TEXT(""));
	RawText.ParseIntoArray(CurrentDialogLines, TEXT("\n"), true);

	if (CurrentDialogLines.Num() == 0)
	{
		CurrentDialogLines.Add(TEXT(""));
	}

	CurrentLineIndex = 0;

	if (CurrentDialogData.DialogSE)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), CurrentDialogData.DialogSE);
	}

	if (CurrentDialogData.DialogVoice)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), CurrentDialogData.DialogVoice);
	}

	if (CurrentDialogData.DialogEmote && CurrentNPC)
	{
		ACharacter* NPCCharacter = Cast<ACharacter>(CurrentNPC);
		if (NPCCharacter)
		{
			NPCCharacter->PlayAnimMontage(CurrentDialogData.DialogEmote);
		}
	}

	ShowCurrentLine();
	return true;
}

void UDialogComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!CurrentDialogData.Choices.IsValidIndex(ChoiceIndex)) return;

	const FDialogChoice& SelectedChoice = CurrentDialogData.Choices[ChoiceIndex];

	bool bStartedFadeNarration = ExecuteActionCore(SelectedChoice.ActionType, SelectedChoice.ActionPayload, SelectedChoice.GrantFlag, SelectedChoice.bFadeOnGrantFlag, SelectedChoice.RemoveFlag, SelectedChoice.bFadeOnRemoveFlag, SelectedChoice.StatToChange, SelectedChoice.StatTargetActor, SelectedChoice.ExtraStatName, SelectedChoice.StatChangeAmount, SelectedChoice.ItemID, SelectedChoice.ItemAmount, SelectedChoice.bAdvanceDailySequence, SelectedChoice.AnimSequenceRowPlayTarget, SelectedChoice.AnimSequenceNPCMeshOverride, SelectedChoice.FadeNarrationText, SelectedChoice.NextDialogID, SelectedChoice.bHideTalkingNPCDuringAnimEvent, SelectedChoice.AdditionalActions);

	HideChoicesUI();
	OnHideChoices.Broadcast();

	// 暗転ナレーションを開始した場合、以降の会話継続（NextDialogIDへ進む/CloseDialog）は
	// 明転完了後にResumeAfterFadeNarrationが行うため、ここでは何もしない
	if (bStartedFadeNarration)
	{
		return;
	}

	FString NextIDStr = SelectedChoice.NextDialogID.ToString().TrimStartAndEnd();
	bool bHasNext = !SelectedChoice.NextDialogID.IsNone() && !NextIDStr.IsEmpty() && NextIDStr.ToLower() != TEXT("none");

	if (bHasNext)
	{
		// 次の行が見つからない（NextDialogIDの設定ミス）場合は、宙ぶらりんにせず会話を閉じる
		if (!TryStartDialog(SelectedChoice.NextDialogID, CurrentTable, CurrentNPC))
		{
			CloseDialog();
		}
	}
	else
	{
		CloseDialog();
	}
}

bool UDialogComponent::ExecuteActionCore(EDialogActionType ActionType, const FString& ActionPayload, FName GrantFlag, bool bFadeOnGrantFlag, FName FlagToRemove, bool bFadeOnRemoveFlag, ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount, FName ItemID, int32 ItemAmount, bool bAdvanceDailySequence, EStatTargetActor AnimSequenceRowPlayTarget, const TSoftObjectPtr<USkeletalMesh>& AnimSequenceNPCMeshOverride, const FText& FadeNarrationText, FName NextDialogIDForNarration, bool bHideTalkingNPCDuringAnimEvent, const TArray<FEventAction>& AdditionalActions)
{

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (!RpgInterface) return false;

	// 他のActionTypeとは異なり、暗転演出そのものを要求して以降の処理（フラグ付与・NextDialogID遷移等）を
	// すべて明転後（ResumeAfterFadeNarration）に委ねるため、ここで単独に処理して抜ける
	if (ActionType == EDialogActionType::ShowTextDuringFade)
	{
		UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance());
		AMyProject1Character* OwnerCharacter = Cast<AMyProject1Character>(GetOwner());
		if (!GameInst || !OwnerCharacter)
		{
			return false;
		}

		FString RawNarration = FadeNarrationText.ToString().Replace(TEXT("\r"), TEXT(""));
		PendingNarrationLines.Empty();
		RawNarration.ParseIntoArray(PendingNarrationLines, TEXT("\n"), true);
		if (PendingNarrationLines.Num() == 0)
		{
			PendingNarrationLines.Add(TEXT(""));
		}
		PendingNarrationNextDialogID = NextDialogIDForNarration;

		// ここでreturnするとStatToChangeの通常処理（関数末尾）に到達しないため、
		// 画面が真っ暗になった瞬間（BeginFadeNarration）に適用できるよう保存しておく
		PendingNarrationStatToChange = StatToChange;
		PendingNarrationStatTargetActor = StatTargetActor;
		PendingNarrationExtraStatName = ExtraStatName;
		PendingNarrationStatChangeAmount = StatChangeAmount;
		PendingNarrationAdditionalActions = AdditionalActions;

		// 暗転演出中（まだ真っ暗になっていない）のAdvanceDialog誤動作を防ぐため、要求と同時に立てる
		bIsFadeNarrationPending = true;

		GameInst->RequestFadeThenShowNarration(this, OwnerCharacter);
		return true;
	}

	// GrantFlagと同じく、ActionTypeが何であっても併用できる独立処理。
	// 話しかけている相手（AQuestNPCBase）の日替わり会話シーケンスを1歩進める
	if (bAdvanceDailySequence)
	{
		if (AQuestNPCBase* QuestNPC = Cast<AQuestNPCBase>(CurrentNPC))
		{
			QuestNPC->AdvanceDailyDialogSequence(Cast<AMyProject1Character>(GetOwner()));
		}
	}

	// ActionType（TalkProgressなど）と併用できる、ActionTypeとは独立したフラグ付与
	if (!GrantFlag.IsNone())
	{
		if (bFadeOnGrantFlag)
		{
			// エリアChangeと同じ暗転を挟んでから付与する（NPCの表示切替などを暗転の裏で行いたい時用）
			if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
			{
				GameInst->RequestFadeThenGrantFlag(GrantFlag, Cast<AMyProject1Character>(GetOwner()));
			}
		}
		else
		{
			RpgInterface->AddFlag(GrantFlag);
		}
	}

	// 行のループAnimEvent（LoopAnimEventID）が再生中の状態で別のアニメーション再生アクションが来た場合の整理。
	// PlayAnimSequenceは新しいAnimEventが古いStepチェーンを置き換えるため、後続のCloseDialog等が新しい方を止めないよう所有権を手放す。
	// PlayAnimSequenceRowはStepチェーン進行中だと無視されるため、先にループを止めておく
	if (ActionType == EDialogActionType::PlayAnimSequence)
	{
		ActiveLoopAnimEventID = NAME_None;
	}
	else if (ActionType == EDialogActionType::PlayAnimSequenceRow)
	{
		StopRowLoopAnimEvent();
	}

	// ActionTypeごとの実行本体はUGameplayActionLibrary（イベント分岐システムと共通）に委譲する
	UGameplayActionLibrary::ExecuteAction(RpgInterface, GetOwner(), CurrentNPC, GetWorld(), ActionType, ActionPayload, ItemID, ItemAmount, AnimSequenceRowPlayTarget, AnimSequenceNPCMeshOverride, bHideTalkingNPCDuringAnimEvent);

	// Warp + NextDialogID：ワープ先で会話を続けたいので、呼び出し元の即時NextDialogID遷移／CloseDialogは行わせず、
	// 明転完了後（HandleWarpFadeInComplete）にResumeAfterFadeNarrationでNextDialogIDから再開する。
	// ワープが予約されなかった場合（RequiredFlag未達等）は従来通り呼び出し元がそのままNextDialogIDへ進む
	bool bWarpResumeReserved = false;
	if (ActionType == EDialogActionType::Warp)
	{
		const FString WarpNextIDStr = NextDialogIDForNarration.ToString().TrimStartAndEnd();
		const bool bWarpHasNext = !NextDialogIDForNarration.IsNone() && !WarpNextIDStr.IsEmpty() && WarpNextIDStr.ToLower() != TEXT("none");
		if (bWarpHasNext)
		{
			if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
			{
				bWarpResumeReserved = GameInst->ReserveDialogResumeAfterWarp(this, NextDialogIDForNarration);
				if (bWarpResumeReserved)
				{
					// 明転中のAdvanceDialog誤動作を防ぐ（ResumeAfterFadeNarrationのTryStartDialog／CloseDialogが解除する）
					bIsFadeNarrationPending = true;
				}
			}
		}
	}

	if (ActionType == EDialogActionType::TriggerEvent)
	{
		// ActionPayloadに設定されたEventPoolIDで、話しかけている相手（CurrentNPC）のイベント抽選を開始する
		// （空欄ならCurrentNPCのEventDistributorComponent自身が持つEventPoolIDにフォールバックする）。
		// AnimSequenceNPCMeshOverrideは、抽選されたイベントがClearCondition=AnimationSequenceで再生する
		// AnimEventIDのExtra参加者（ExtraPairings）のメッシュ差し替えに使う
		if (CurrentNPC)
		{
			if (UEventDistributorComponent* EventComp = CurrentNPC->FindComponentByClass<UEventDistributorComponent>())
			{
				// FName(TEXT(""))はNAME_Noneと等しくならないため、空欄判定は先にFString側で行う
				const FName EventPoolIDOverride = ActionPayload.IsEmpty() ? NAME_None : FName(*ActionPayload);
				EventComp->TriggerEventPool(Cast<AMyProject1Character>(GetOwner()), EventPoolIDOverride, AnimSequenceNPCMeshOverride, bHideTalkingNPCDuringAnimEvent);
			}
		}
	}
	else if (ActionType == EDialogActionType::Close)
	{
		CloseDialog();
	}

	// GrantFlagと同じく、ActionType（Warpなど）と併用できる、ActionTypeとは独立したフラグ消去。
	// switch(Warp等)より後に実行することで、暗転が絡む場合にRequestFadeThenRemoveFlag側から
	// 「Warpの暗転に既に予約が乗っているか」を判定できるようにしている（二重に暗転させないため）
	// （例：バイトを終了してワープで部屋を出る選択肢で、開始時に立てたフラグをここで消す）
	if (!FlagToRemove.IsNone())
	{
		if (bFadeOnRemoveFlag)
		{
			// エリアChangeと同じ暗転を挟んでから消去する（NPCの表示切替を暗転の裏で行いたい時用。
			// 直前のWarpで既に暗転が予約されていれば、そちらに相乗りして二重に暗転しない）
			if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
			{
				GameInst->RequestFadeThenRemoveFlag(FlagToRemove, Cast<AMyProject1Character>(GetOwner()));
			}
		}
		else
		{
			RpgInterface->RemoveFlag(FlagToRemove);
		}
	}

	// StatToChangeによるステータス変化本体もUGameplayActionLibraryに委譲する（ActionTypeとは独立して併用可能）
	UGameplayActionLibrary::ApplyStatChange(RpgInterface, CurrentNPC, StatToChange, StatTargetActor, ExtraStatName, StatChangeAmount);

	ExecuteAdditionalActions(AdditionalActions);

	return bWarpResumeReserved;
}

// ----------------------------------------------------
// 行ごとのループAnimEvent（FDialogData::LoopAnimEventID）
// ----------------------------------------------------

void UDialogComponent::StartRowLoopAnimEvent(FName LoopAnimEventID)
{
	if (LoopAnimEventID.IsNone()) return;

	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance());
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!GameInst || !OwnerChar) return;

	// 直前の行と同じIDが再生中なら止めずにそのまま継続する（Durationが尽きて既に終わっていた場合は再生し直す）
	if (LoopAnimEventID == ActiveLoopAnimEventID && GameInst->IsPlayingAnimSequenceEventFor(OwnerChar))
	{
		return;
	}

	// 別のIDに切り替える場合は、先に今のループを正規の後片付け（ループ中モンタージュの停止・位置/Extra参加者の復元）で止める
	StopRowLoopAnimEvent();

	// 会話の表示と同時にその場で始め（開始前の暗転は挟まない）、止められるまで姿勢を保持する。
	// DT_AnimEvents側のStep設定（bLoop/Duration）による時間切れ・次ステップへの暗転遷移は起こさない
	GameInst->PlayAnimSequenceEventHeld(LoopAnimEventID, OwnerChar, CurrentNPC);

	// 開始に失敗した場合（AnimEventID未登録等）は再生中にならないため、所有権を持たない
	ActiveLoopAnimEventID = GameInst->IsPlayingAnimSequenceEventFor(OwnerChar) ? LoopAnimEventID : NAME_None;
}

void UDialogComponent::StopRowLoopAnimEvent()
{
	if (ActiveLoopAnimEventID.IsNone()) return;
	ActiveLoopAnimEventID = NAME_None;

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInst->StopAnimSequenceEventImmediately();
	}
}

void UDialogComponent::ExecuteAdditionalActions(const TArray<FEventAction>& Actions)
{
	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (!RpgInterface) return;

	for (const FEventAction& Action : Actions)
	{
		UGameplayActionLibrary::ExecuteAction(RpgInterface, GetOwner(), CurrentNPC, GetWorld(),
			Action.ActionType, Action.ActionPayload, Action.ItemID, Action.ItemAmount);
		UGameplayActionLibrary::ApplyStatChange(RpgInterface, CurrentNPC,
			Action.StatToChange, Action.StatTargetActor, Action.ExtraStatName, Action.StatChangeAmount);
	}
}

void UDialogComponent::CloseDialog()
{
	// 暗転ナレーション状態が残ったまま会話が閉じることのないよう、念のためここでもリセットする（保険）
	bIsFadeNarrationPending = false;
	bIsShowingFadeNarration = false;

	// ループ用AnimEventが再生中なら、下のIsPlayingAnimSequenceEventForの判定より前に止めておく
	// （止めずに判定すると「再生中」と見なされ、入力ロックが解除されないまま会話が閉じてしまう）
	StopRowLoopAnimEvent();

	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
	{
		RpgInterface->CancelTarget();

		// PlayAnimSequence選択肢（NextDialogID未設定）はExecuteActionCore→PlayAnimSequenceEventが
		// 直後にSetInputLocked(true)をかけている。また、TriggerEvent選択肢（EventDistributorComponent経由）は
		// StartEvent→ワープの暗転待ちを挟んでからPlayAnimSequenceEventを開始するため、CloseDialogが呼ばれる
		// 時点ではまだPlayAnimSequenceEvent自体は始まっていないことがある。どちらの場合も、ここで無条件に
		// falseへ戻すとロックが解除・またはこれから始まるロックが台無しになるため、アニメイベント再生中
		// （またはこれから始まる予定）はロック解除しない（解除はPlayAnimEventStep側の完了時に任せる）
		bool bAnimSequenceEventPlaying = false;
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
		{
			bAnimSequenceEventPlaying = GameInst->IsPlayingAnimSequenceEventFor(Cast<ACharacter>(GetOwner()))
				|| GameInst->IsActiveEventPendingAnimationSequence();
		}

		if (!bAnimSequenceEventPlaying)
		{
			RpgInterface->SetInputLocked(false);
		}
	}

	// コントローラーの取得は APawn（キャラクターの親玉）としての標準機能を使う
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
	}

	CloseDialogUI();
	OnDialogClosed.Broadcast();
}

void UDialogComponent::AdvanceDialog()
{
	// 暗転ナレーション表示中はこちらへ分岐する。暗転演出中（まだ真っ暗になっていない、
	// bIsFadeNarrationPending中）は、古いCurrentDialogLinesで通常の会話処理が誤って進まないよう何もしない
	if (bIsShowingFadeNarration)
	{
		AdvanceFadeNarrationLine();
		return;
	}
	if (bIsFadeNarrationPending)
	{
		return;
	}

	if (CurrentLineIndex < CurrentDialogLines.Num() - 1)
	{
		CurrentLineIndex++;
		ShowCurrentLine();
		return;
	}

	// 最終ページで選択肢が出ている場合、「クリック/スペースで進む」は無効にする。
	// 選択肢の進行はSelectChoice経由のみ。ここを通すと、前面のWBP_DialogAdvanceへのクリック透過や
	// キーフォーカス残りで、選択せずに会話が閉じてしまう（Questも受注されない）事故になる
	if (CurrentDialogData.Choices.Num() > 0)
	{
		return;
	}

	if (CurrentDialogData.bEndDialog || CurrentDialogData.NextDialogID.IsNone())
	{
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
			{
				if (IsValid(this))
				{
					CloseDialog();
				}
			}, 0.01f, false);
	}
	else if (!TryStartDialog(CurrentDialogData.NextDialogID, CurrentTable, CurrentNPC))
	{
		// 次の行が見つからない（NextDialogIDの設定ミス）場合は、宙ぶらりんにせず会話を閉じる
		CloseDialog();
	}
}

bool UDialogComponent::AreChoicesActive() const
{
	// 暗転ナレーション中/開始待ち中は、暗転前の選択肢データが残っていても入力待ちとみなさない
	if (bIsShowingFadeNarration || bIsFadeNarrationPending)
	{
		return false;
	}

	// ShowCurrentLineは「最終ページ以外」ではChoicesを空にして送るので、
	// 最終ページ かつ Choicesが1件以上 の時だけ「選択肢の入力待ち」とみなす
	return CurrentDialogData.Choices.Num() > 0
		&& CurrentDialogLines.Num() > 0
		&& CurrentLineIndex == CurrentDialogLines.Num() - 1;
}

bool UDialogComponent::CanSelectChoice(const FDialogChoice& Choice) const
{
	
	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (!RpgInterface) return false;

	if (Choice.bUseStatCondition)
	{
		float StatValue = 0.0f;
		if (!UGameplayActionLibrary::TryGetTargetStatValue(RpgInterface, CurrentNPC, Choice.RequiredStat, Choice.RequiredStatTargetActor, Choice.RequiredStatExtraName, StatValue))
		{
			return false;
		}

		bool bStatConditionMet = false;
		switch (Choice.RequiredStatCompareOp)
		{
		case EStatCompareOp::GreaterOrEqual: bStatConditionMet = StatValue >= Choice.RequiredStatValue; break;
		case EStatCompareOp::LessOrEqual:    bStatConditionMet = StatValue <= Choice.RequiredStatValue; break;
		case EStatCompareOp::Equal:          bStatConditionMet = FMath::IsNearlyEqual(StatValue, Choice.RequiredStatValue); break;
		case EStatCompareOp::Greater:        bStatConditionMet = StatValue > Choice.RequiredStatValue; break;
		case EStatCompareOp::Less:           bStatConditionMet = StatValue < Choice.RequiredStatValue; break;
		}
		if (!bStatConditionMet)
		{
			return false;
		}
	}

	if (!Choice.RequiredFlag.IsNone())
	{
		if (!RpgInterface->HasFlag(Choice.RequiredFlag))
		{
			return false;
		}
	}

	if (Choice.RequiredGil > 0)
	{
		const UInventoryComponent* Inv = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
		if (!Inv || Inv->Gil < Choice.RequiredGil)
		{
			return false;
		}
	}

	return true;
}

void UDialogComponent::ShowCurrentLine()
{
	if (!CurrentDialogLines.IsValidIndex(CurrentLineIndex)) return;

	FDialogData DisplayData = CurrentDialogData;
	DisplayData.DialogText = FText::FromString(CurrentDialogLines[CurrentLineIndex]);

	if (CurrentLineIndex < CurrentDialogLines.Num() - 1)
	{
		DisplayData.Choices.Empty();
		DisplayData.NextDialogID = FName(TEXT("DummyNextPage"));
		DisplayData.bEndDialog = false;
	}


	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
			{
				if (HUD->DialogLineSound)
				{
					UGameplayStatics::PlaySound2D(GetWorld(), HUD->DialogLineSound);
				}
			}
		}
	}

	// ShowTextDuringFadeの最終ページ（選択肢なし）は暗転演出そのものが目的の行で、
	// DialogTextは空欄運用（実際のセリフはFadeNarrationText側）。通常のOnDialogUpdatedを流すと、
	// UI側のログ機能等に「NPC名だけ・セリフ空」の行が残ってしまうため、この行だけは通常表示を出さない。
	// ただし、最終ページに本文が入っている場合は通常のセリフとして表示する（例：4行のセリフの最後で暗転セリフへ移る行は、4行目も出す）
	bool bIsFinalFadeNarrationTrigger = (CurrentLineIndex == CurrentDialogLines.Num() - 1)
		&& CurrentDialogData.Choices.Num() == 0
		&& CurrentDialogData.ActionType == EDialogActionType::ShowTextDuringFade
		&& CurrentDialogLines[CurrentLineIndex].TrimStartAndEnd().IsEmpty();

	if (!bIsFinalFadeNarrationTrigger)
	{
		UpdateDialogUI(DisplayData, CurrentNPC);
		OnDialogUpdated.Broadcast(DisplayData, CurrentNPC);
	}

	if (CurrentLineIndex == CurrentDialogLines.Num() - 1)
	{
		// 選択肢がない会話は、このセリフに直接設定されたアクションをここで実行する
		// （会話終了設定の場合、この下でAdvanceDialogを経由せず直接CloseDialogするため、ここで実行しないと機会を失う）
		if (CurrentDialogData.Choices.Num() == 0)
		{
			bool bStartedFadeNarration = ExecuteActionCore(CurrentDialogData.ActionType, CurrentDialogData.ActionPayload, CurrentDialogData.GrantFlag, CurrentDialogData.bFadeOnGrantFlag, CurrentDialogData.RemoveFlag, CurrentDialogData.bFadeOnRemoveFlag, CurrentDialogData.StatToChange, CurrentDialogData.StatTargetActor, CurrentDialogData.ExtraStatName, CurrentDialogData.StatChangeAmount, CurrentDialogData.ItemID, CurrentDialogData.ItemAmount, CurrentDialogData.bAdvanceDailySequence, EStatTargetActor::Player, CurrentDialogData.AnimSequenceNPCMeshOverride, CurrentDialogData.FadeNarrationText, CurrentDialogData.NextDialogID, CurrentDialogData.bHideTalkingNPCDuringAnimEvent, CurrentDialogData.AdditionalActions);

			// 暗転ナレーションを開始した場合、以降（CloseDialog判定含む）は明転完了後の
			// ResumeAfterFadeNarrationに委ねるため、ここで抜ける
			if (bStartedFadeNarration)
			{
				return;
			}
		}

		if (CurrentDialogData.Choices.Num() == 0 &&
			(CurrentDialogData.bEndDialog || CurrentDialogData.NextDialogID.IsNone()))
		{
			FTimerHandle TimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
				{
					if (IsValid(this))
					{
						CloseDialog();
					}
				}, 0.05f, false);
		}
	}
}

// ----------------------------------------------------
// 暗転中ナレーション（ShowTextDuringFade）
// ----------------------------------------------------

void UDialogComponent::BeginFadeNarration()
{
	bIsFadeNarrationPending = false;
	bIsShowingFadeNarration = true;

	CurrentDialogLines = PendingNarrationLines;
	CurrentLineIndex = 0;

	// 画面が真っ暗で見えない今のうちに、ループ用AnimEventを切り替える。
	// 続く行にLoopAnimEventIDがあればそれ、無ければこの暗転セリフ行（または選択肢の元の行）自身の指定を使う。
	// どちらも空なら今のループをそのまま継続し、同じIDが再生中なら止めずに継続する（会話が閉じる時にCloseDialogが止める）
	FName DesiredLoopAnimEventID = CurrentDialogData.LoopAnimEventID;
	if (CurrentTable && !PendingNarrationNextDialogID.IsNone())
	{
		if (const FDialogData* NextRow = CurrentTable->FindRow<FDialogData>(PendingNarrationNextDialogID, TEXT("BeginFadeNarration"), false))
		{
			if (!NextRow->LoopAnimEventID.IsNone())
			{
				DesiredLoopAnimEventID = NextRow->LoopAnimEventID;
			}
		}
	}
	StartRowLoopAnimEvent(DesiredLoopAnimEventID);

	// 画面が真っ暗な今のうちに、保存しておいたステータス変化を適用する
	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
	{
		UGameplayActionLibrary::ApplyStatChange(RpgInterface, CurrentNPC, PendingNarrationStatToChange, PendingNarrationStatTargetActor, PendingNarrationExtraStatName, PendingNarrationStatChangeAmount);
	}
	PendingNarrationStatToChange = ETargetStat::None;
	PendingNarrationStatChangeAmount = 0.0f;

	// 追加アクションは、ナレーションより先にログが出て読む順序が迷子にならないよう、
	// 全行読み終えた時点（AdvanceFadeNarrationLine）で実行する

	ShowFadeNarrationLine();
}

void UDialogComponent::ShowFadeNarrationLine()
{
	if (!CurrentDialogLines.IsValidIndex(CurrentLineIndex)) return;

	// 暗転セリフの行から会話を始めた場合など、この時点で「進む」ウィジェットがまだ無いことがある
	// （従来は「次の行がある」セリフを表示した時にしか作られず、無いとクリック／スペースを受ける先が無く1行目で止まっていた）。
	// 会話が起点の暗転ナレーションなので、ここで確実に用意してフォーカスを渡す
	EnsureDialogAdvanceWidget();
	if (DialogAdvanceWidget)
	{
		ApplyDialogInputMode(DialogAdvanceWidget);
	}

	OnFadeNarrationLine.Broadcast(FText::FromString(CurrentDialogLines[CurrentLineIndex]));
}

void UDialogComponent::AdvanceFadeNarrationLine()
{
	if (CurrentLineIndex < CurrentDialogLines.Num() - 1)
	{
		CurrentLineIndex++;
		ShowFadeNarrationLine();
		return;
	}

	// 全行読み終えた：ナレーション表示を終了し、GameInstance側に明転再開を要求する。
	// 明転が完了した瞬間、GameInstance::HandleWarpFadeInCompleteからResumeAfterFadeNarrationが呼ばれる
	bIsShowingFadeNarration = false;

	// ナレーションを読み終えた今（まだ画面は真っ暗）、追加アクション（アイテム使用など）を実行する
	ExecuteAdditionalActions(PendingNarrationAdditionalActions);
	PendingNarrationAdditionalActions.Empty();

	// 明転が終わってResumeAfterFadeNarrationが呼ばれるまでの間は、AdvanceDialogを受け付けない。
	// ここでfalseのままだと、明転中にもう一度クリック/スペースが入った時に通常の会話処理へ流れ、
	// CurrentDialogData.NextDialogIDの行をこちらで先に開始してしまい、その後のResumeAfterFadeNarrationと二重に開始される。
	// このフラグはResumeAfterFadeNarrationが呼ぶTryStartDialog／CloseDialogが必ず解除する
	bIsFadeNarrationPending = true;

	OnFadeNarrationClosed.Broadcast();

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInst->ResumeFadeInAfterNarration(this, PendingNarrationNextDialogID);
	}
}

void UDialogComponent::ResumeAfterFadeNarration(FName NextDialogID)
{
	FString NextIDStr = NextDialogID.ToString().TrimStartAndEnd();
	bool bHasNext = !NextDialogID.IsNone() && !NextIDStr.IsEmpty() && NextIDStr.ToLower() != TEXT("none");

	if (bHasNext)
	{
		// 次の行が見つからない（NextDialogIDの設定ミス）場合は、宙ぶらりんにせず会話を閉じる
		if (!TryStartDialog(NextDialogID, CurrentTable, CurrentNPC))
		{
			CloseDialog();
		}
	}
	else
	{
		CloseDialog();
	}
}

// ----------------------------------------------------
// 会話UI（旧BP_ThirdPersonCharacterのOnDialogUpdated/OnDialogClosed/OnHideChoicesイベントの移植）
// ----------------------------------------------------

void UDialogComponent::BeginPlay()
{
	Super::BeginPlay();

	// GameInstanceがイベント開始/終了ナレーションや睡眠ナレーションでも直接BroadcastするOnFadeNarrationLine/Closedを、
	// 暗転ナレーション用ウィジェットの更新に使う（会話起点のナレーションも同じ経路を通る）
	OnFadeNarrationLine.AddUniqueDynamic(this, &UDialogComponent::HandleFadeNarrationLineUI);
	OnFadeNarrationClosed.AddUniqueDynamic(this, &UDialogComponent::HandleFadeNarrationClosedUI);
}

APlayerController* UDialogComponent::GetOwnerPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

AMyProject1HUD* UDialogComponent::GetOwnerHUD() const
{
	const APlayerController* PC = GetOwnerPlayerController();
	return PC ? Cast<AMyProject1HUD>(PC->GetHUD()) : nullptr;
}

void UDialogComponent::ApplyDialogInputMode(UUserWidget* WidgetToFocus)
{
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		FInputModeGameAndUI InputMode;
		if (WidgetToFocus)
		{
			InputMode.SetWidgetToFocus(WidgetToFocus->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
	{
		RpgInterface->SetInputLocked(true);
	}
}

void UDialogComponent::EnsureDialogAdvanceWidget()
{
	if (IsValid(DialogAdvanceWidget) && DialogAdvanceWidget->IsInViewport())
	{
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	const AMyProject1HUD* HUD = GetOwnerHUD();
	if (!PC || !HUD || !HUD->DialogAdvanceClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialog] HUDの DialogAdvanceClass が未設定のため、会話を進めるウィジェットを作れません。BP_HUDのクラスデフォルトで WBP_DialogAdvance を指定してください。"));
		return;
	}

	if (!IsValid(DialogAdvanceWidget))
	{
		DialogAdvanceWidget = CreateWidget<UWBP_DialogAdvance>(PC, HUD->DialogAdvanceClass);
	}
	if (DialogAdvanceWidget)
	{
		DialogAdvanceWidget->AddToViewport();
	}
}

void UDialogComponent::UpdateDialogUI(const FDialogData& DisplayData, AActor* NPC)
{
	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());

	// クエストNPC相手の会話だけ、「NPC名：セリフ」をログへ出す（従来のBPと同じ仕様。SpeakerNameは使わない）
	if (const AQuestNPCBase* QuestNPC = Cast<AQuestNPCBase>(NPC))
	{
		if (RpgInterface)
		{
			RpgInterface->OnReceiveLogMessage(QuestNPC->NPCName + TEXT("：") + DisplayData.DialogText.ToString(), ELogMessageType::Default);
		}

		// このセリフで会話を終える指定なら、選択肢・「進む」ウィジェットは出さずにUIを片付ける
		if (DisplayData.bEndDialog)
		{
			CloseDialogUI();
			return;
		}
	}

	if (DisplayData.Choices.Num() > 0)
	{
		APlayerController* PC = GetOwnerPlayerController();
		const AMyProject1HUD* HUD = GetOwnerHUD();
		if (!PC || !HUD || !HUD->DialogChoiceMenuClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Dialog] HUDの DialogChoiceMenuClass が未設定のため、選択肢を表示できません。BP_HUDのクラスデフォルトで WBP_ChoiceMenu を指定してください。"));
			return;
		}

		if (!IsValid(ChoiceMenuWidget))
		{
			ChoiceMenuWidget = CreateWidget<UWBP_ChoiceMenu>(PC, HUD->DialogChoiceMenuClass);
		}
		if (ChoiceMenuWidget)
		{
			if (!ChoiceMenuWidget->IsInViewport())
			{
				ChoiceMenuWidget->AddToViewport();
			}
			ChoiceMenuWidget->RefreshChoices(DisplayData.Choices);
			ApplyDialogInputMode(ChoiceMenuWidget);
		}
	}
	else if (!DisplayData.NextDialogID.IsNone())
	{
		// 次の行がある（クリック／スペースで進める）セリフ。NextDialogIDが空のセリフは、ShowCurrentLineが自動で会話を閉じる
		EnsureDialogAdvanceWidget();
		if (DialogAdvanceWidget)
		{
			ApplyDialogInputMode(DialogAdvanceWidget);
		}
	}

	// 既に出ている「進む」ウィジェットは、選択肢の入力待ちの間だけ隠す（選択肢ボタンへクリックを通すため）
	if (IsValid(DialogAdvanceWidget) && DialogAdvanceWidget->IsInViewport())
	{
		DialogAdvanceWidget->UpdateAdvanceVisibility();
	}
}

void UDialogComponent::CloseDialogUI()
{
	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
	{
		RpgInterface->SetInputLocked(false);
	}

	if (IsValid(ChoiceMenuWidget))
	{
		ChoiceMenuWidget->RemoveFromParent();
	}
	ChoiceMenuWidget = nullptr;

	if (IsValid(DialogAdvanceWidget))
	{
		DialogAdvanceWidget->RemoveFromParent();
	}
	DialogAdvanceWidget = nullptr;

	if (APlayerController* PC = GetOwnerPlayerController())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}

void UDialogComponent::HideChoicesUI()
{
	if (IsValid(ChoiceMenuWidget))
	{
		ChoiceMenuWidget->HideChoices();
	}
}

void UDialogComponent::HandleFadeNarrationLineUI(const FText& Line)
{
	if (!IsValid(FadeNarrationWidget))
	{
		APlayerController* PC = GetOwnerPlayerController();
		const AMyProject1HUD* HUD = GetOwnerHUD();
		if (!PC || !HUD || !HUD->FadeNarrationClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Dialog] HUDの FadeNarrationClass が未設定のため、暗転ナレーションを表示できません。BP_HUDのクラスデフォルトで WBP_FadeNarration を指定してください。"));
			return;
		}
		FadeNarrationWidget = CreateWidget<UWBP_FadeNarration>(PC, HUD->FadeNarrationClass);
	}

	if (FadeNarrationWidget)
	{
		if (!FadeNarrationWidget->IsInViewport())
		{
			FadeNarrationWidget->AddToViewport(10);
		}
		FadeNarrationWidget->ShowLine(Line);
	}

	// 「進む」ウィジェットが既にあれば、ナレーション中は必ず見える状態にしてキー入力を受けさせる
	if (IsValid(DialogAdvanceWidget) && DialogAdvanceWidget->IsInViewport())
	{
		DialogAdvanceWidget->UpdateAdvanceVisibility();
		DialogAdvanceWidget->RequestAdvanceFocus();
	}
}

void UDialogComponent::HandleFadeNarrationClosedUI()
{
	if (IsValid(FadeNarrationWidget))
	{
		FadeNarrationWidget->HideNarration();
	}
}
