#include "MyProject1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "MyProject1Character.h"
#include "MyProject1SaveGame.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "SkinOverlayComponent.h"
#include "GameFramework/PlayerController.h"
#include "WallWarpLink.h"
#include "GameplayActionLibrary.h"
#include "RpgCharacterInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MusicControlComponent.h"
#include "AnimEventActor.h"
#include "QuestNPCBase.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "DialogComponent.h"


void UMyProject1GameInstance::Init()
{
	Super::Init();

	// 現実の1秒ごとに UpdateInGameTime を呼ぶタイマーをセット
	GetTimerManager().SetTimer(TimeUpdateTimerHandle, this, &UMyProject1GameInstance::UpdateInGameTime, RealSecondsPerGameMinute, true);
}

void UMyProject1GameInstance::UpdateInGameTime()
{
	// 例：現実の1秒で、ゲーム内の1分が進む設計（倍率は自由に調整可）
	CurrentTimeInMinutes += 1;

	// 24時間（1440分）を超えたら0に戻す（翌日）
	if (CurrentTimeInMinutes >= 1440)
	{
		CurrentTimeInMinutes = 0;
		AdvanceDay();
	}

	// 「時」と「分」を計算
	int32 Hour = CurrentTimeInMinutes / 60;
	int32 Minute = CurrentTimeInMinutes % 60;

	// UIに向けて「時間が変わったよ！」とお知らせする
	if (OnInGameTimeChanged.IsBound())
	{
		OnInGameTimeChanged.Broadcast(CurrentYear, CurrentMonth, CurrentDay, Hour, Minute);
	}
}

// 待機/睡眠などで時間を一気に進める（分単位）
void UMyProject1GameInstance::AdvanceTimeBy(int32 MinutesToAdd)
{
	if (MinutesToAdd <= 0)
	{
		return;
	}

	CurrentTimeInMinutes += MinutesToAdd;

	// 日をまたぐ分だけAdvanceDayを個別に呼ぶ（複数日またぐ待機でも1日ずつ正しく進む）
	while (CurrentTimeInMinutes >= 1440)
	{
		CurrentTimeInMinutes -= 1440;
		AdvanceDay();
	}

	int32 Hour = CurrentTimeInMinutes / 60;
	int32 Minute = CurrentTimeInMinutes % 60;

	if (OnInGameTimeChanged.IsBound())
	{
		OnInGameTimeChanged.Broadcast(CurrentYear, CurrentMonth, CurrentDay, Hour, Minute);
	}
}

// 日付を1日進める処理
void UMyProject1GameInstance::AdvanceDay()
{
	CurrentDay++;
	TotalElapsedDays++;

	// その月の最終日を超えたら、翌月の1日にする
	if (CurrentDay > GetDaysInMonth(CurrentYear, CurrentMonth))
	{
		CurrentDay = 1;
		CurrentMonth++;

		// 12月を超えたら、翌年の1月にする
		if (CurrentMonth > 12)
		{
			CurrentMonth = 1;
			CurrentYear++;
		}
	}

	if (OnDayChangedDelegate.IsBound())
	{
		OnDayChangedDelegate.Broadcast();
	}
}

// 指定した「年・月」の日数を計算する処理
int32 UMyProject1GameInstance::GetDaysInMonth(int32 Year, int32 Month)
{
	// 1月〜12月までの各月の日数リスト（0番目は空っぽ）
	static const int32 DaysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if (Month == 2)
	{
		// 2月の場合：閏年（うるうどし）の計算を行う
		bool bIsLeapYear = (Year % 4 == 0 && Year % 100 != 0) || (Year % 400 == 0);
		return bIsLeapYear ? 29 : 28;
	}

	// 正しい月の範囲なら配列から日数を返し、エラー値なら安全のため31を返す
	return (Month >= 1 && Month <= 12) ? DaysPerMonth[Month] : 31;
}

// ----------------------------------------------------
// 1. ワープの「要求」と「暗転の開始」
// ----------------------------------------------------
void UMyProject1GameInstance::RequestWarp(FName WarpID, ACharacter* PlayerCharacter, bool bBypassRequiredFlag)
{
	if (!PlayerCharacter) { UE_LOG(LogTemp, Warning, TEXT("RequestWarp: PlayerCharacter is null")); return; }
	if (!WarpDataTable) { UE_LOG(LogTemp, Warning, TEXT("RequestWarp: WarpDataTable is not set on GameInstance")); return; }
	if (WarpID.IsNone()) { UE_LOG(LogTemp, Warning, TEXT("RequestWarp: WarpID is None")); return; }

	FWarpDestination* WarpData = WarpDataTable->FindRow<FWarpDestination>(WarpID, TEXT("WarpContext"));
	if (!WarpData)
	{
		UE_LOG(LogTemp, Warning, TEXT("WarpID: %s not found!"), *WarpID.ToString());
		return;
	}

	AMyProject1Character* MyChar = Cast<AMyProject1Character>(PlayerCharacter);
	if (!bBypassRequiredFlag && MyChar && !WarpData->RequiredFlag.IsNone())
	{
		if (!MyChar->HasFlag(WarpData->RequiredFlag))
		{
			const FString Msg = WarpData->RequiredFlagMessage.IsEmpty() ? TEXT("何か目に見えない力に阻まれて、先に進めない…。") : WarpData->RequiredFlagMessage;
			MyChar->OnReceiveLogMessage(Msg, ELogMessageType::System);
			return;
		}
	}

	// --- ★変更：ここから下を「暗転開始」の処理に書き換えます ---

	// 後で移動できるように情報を予約しておく
	ReservedWarpID = WarpID;
	ReservedPlayer = PlayerCharacter;

	BeginWarpFade(PlayerCharacter);
}

// ----------------------------------------------------
// 1.5. ワープを伴わない「暗転を挟んだフラグ付与」の要求（NPCの表示切替など向け）
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenGrantFlag(FName FlagName, AMyProject1Character* TargetCharacter)
{
	if (FlagName.IsNone() || !TargetCharacter) return;

	ReservedFlagToGrant = FlagName;
	ReservedFlagGrantTarget = TargetCharacter;

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.5.5. ワープを伴わない「暗転を挟んだフラグ消去」の要求（NPCの表示切替など向け）
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenRemoveFlag(FName FlagName, AMyProject1Character* TargetCharacter)
{
	if (FlagName.IsNone() || !TargetCharacter) return;

	ReservedFlagToRemove = FlagName;
	ReservedFlagRemoveTarget = TargetCharacter;

	// 同じ選択肢のActionType=Warp（またはWallWarp）が既にこのフレームで暗転を予約済みなら、
	// そちらの暗転にそのまま相乗りする（二重にOnWarpFadeOutRequestedを鳴らして暗転を壊さないため）。
	// フラグの消去自体はExecuteWarpProcessの先頭で必ず反映されるので、ここでreturnしても消え忘れない
	if (!ReservedWarpID.IsNone() || ReservedWallWarpLink.IsValid())
	{
		return;
	}

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.5.6. 待機/睡眠による「暗転を挟んだ時間スキップ」の要求
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenAdvanceTime(int32 MinutesToAdd, ACharacter* TargetCharacter, bool bIsSleep)
{
	if (MinutesToAdd <= 0 || !TargetCharacter) return;

	ReservedTimeSkipMinutes = MinutesToAdd;
	ReservedTimeSkipCharacter = TargetCharacter;
	ReservedTimeSkipIsSleep = bIsSleep;

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.6. WallWarpLink用の「暗転を挟んだ軽量ワープ」の要求
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenWallWarp(AWallWarpLink* SourceLink, ACharacter* TargetCharacter)
{
	if (!SourceLink || !TargetCharacter) return;

	ReservedWallWarpLink = SourceLink;
	ReservedWallWarpCharacter = TargetCharacter;

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.6.5. UDialogComponent（ActionType=ShowTextDuringFade）用の「暗転を挟んだナレーション表示」の要求
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenShowNarration(UDialogComponent* NarrationComponent, ACharacter* TargetCharacter)
{
	if (!NarrationComponent || !TargetCharacter) return;

	ReservedNarrationComponent = NarrationComponent;

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.6.6. ナレーション全行読了後、UDialogComponentから呼ばれる明転再開の要求
// ----------------------------------------------------
void UMyProject1GameInstance::ResumeFadeInAfterNarration(UDialogComponent* NarrationComponent, FName NextDialogID)
{
	// HandleWarpFadeOutCompleteが保留していたFadeInタイマーを、ここで初めて開始する
	if (!bWaitingForNarrationCompletion) return;
	bWaitingForNarrationCompletion = false;

	ReservedNarrationResumeComponent = NarrationComponent;
	ReservedNarrationResumeNextDialogID = NextDialogID;

	// WBP_LoadingScreen側へ「暗転アニメーション終了時に止めていた自動明転を、今開始してよい」と合図する
	OnNarrationReadyToFadeIn.Broadcast();

	GetTimerManager().SetTimer(WarpFadeInTimerHandle, this,
		&UMyProject1GameInstance::HandleWarpFadeInComplete, WarpFadeInDuration, false);
}

// ----------------------------------------------------
// 1.7. 暗転演出の共通処理（入力停止→暗転タイマー予約→UIへ合図）
// ----------------------------------------------------
void UMyProject1GameInstance::BeginWarpFade(ACharacter* TargetCharacter)
{
	if (!TargetCharacter) return;

	if (APlayerController* PC = Cast<APlayerController>(TargetCharacter->GetController()))
	{
		TargetCharacter->DisableInput(PC);
	}
	InputDisabledCharacter = TargetCharacter;

	// 同じフレームで複数のRequest系が呼ばれても（フラグ消去の相乗り等）、暗転タイマーは1本だけ動かす
	if (!GetTimerManager().IsTimerActive(WarpFadeOutTimerHandle))
	{
		GetTimerManager().SetTimer(WarpFadeOutTimerHandle, this,
			&UMyProject1GameInstance::HandleWarpFadeOutComplete, WarpFadeOutDuration, false);
	}

	// UI（Widget）へ「暗転開始アニメーションを再生して！」と合図を送る
	OnWarpFadeOutRequested.Broadcast();
}

// ----------------------------------------------------
// 1.8. 暗転が終わった（＝画面が真っ暗になった）タイミングで自動的に呼ばれる
// ----------------------------------------------------
void UMyProject1GameInstance::HandleWarpFadeOutComplete()
{
	ExecuteWarpProcess();

	// ShowTextDuringFadeのナレーション表示中は、プレイヤーが全行読み終えるまで明転させない。
	// ResumeFadeInAfterNarrationが呼ばれた時点で改めてこのタイマーをセットする
	if (bWaitingForNarrationCompletion)
	{
		return;
	}

	GetTimerManager().SetTimer(WarpFadeInTimerHandle, this,
		&UMyProject1GameInstance::HandleWarpFadeInComplete, WarpFadeInDuration, false);
}

// ----------------------------------------------------
// 1.9. 明転が終わったタイミングで自動的に呼ばれ、入力を戻す
// ----------------------------------------------------
void UMyProject1GameInstance::HandleWarpFadeInComplete()
{
	if (ACharacter* Character = InputDisabledCharacter.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			Character->EnableInput(PC);
		}
	}
	InputDisabledCharacter.Reset();

	// ShowTextDuringFadeのナレーション読了後の明転であれば、通常のBeginAnimEventSequenceIfNeededではなく
	// DialogComponent側にNextDialogIDへの会話継続（またはCloseDialog）を委ねる
	if (UDialogComponent* NarrationComponent = ReservedNarrationResumeComponent.Get())
	{
		FName NextDialogID = ReservedNarrationResumeNextDialogID;
		ReservedNarrationResumeComponent.Reset();
		ReservedNarrationResumeNextDialogID = NAME_None;

		NarrationComponent->ResumeAfterFadeNarration(NextDialogID);
		return;
	}

	BeginAnimEventSequenceIfNeeded();
}

// ----------------------------------------------------
// 2. 暗転が終わった後に呼ばれる「実際の移動」
// ----------------------------------------------------
void UMyProject1GameInstance::ExecuteWarpProcess()
{
	// フラグ消去の予約は、Warp/WallWarp等の他の予約と同じ暗転に相乗りしている可能性があるため、
	// ここではreturnせず必ず先に消化してから、下の各分岐（Warp本来の移動など）へ続ける
	if (!ReservedFlagToRemove.IsNone())
	{
		if (AMyProject1Character* Character = ReservedFlagRemoveTarget.Get())
		{
			Character->RemoveFlag(ReservedFlagToRemove);
		}

		ReservedFlagToRemove = NAME_None;
		ReservedFlagRemoveTarget.Reset();
	}

	// 待機/睡眠による時間スキップの予約があれば、こちらで完結させる（RequestFadeThenAdvanceTime用）
	if (ReservedTimeSkipMinutes > 0)
	{
		AdvanceTimeBy(ReservedTimeSkipMinutes);

		// HandleFatigueTickは現実時間の経過にしか反応しないため、待機/睡眠でジャンプした分の疲労度は
		// ここで明示的に反映する（そうしないと待機による時間経過分の疲労上昇が抜け落ちる）
		if (AMyProject1Character* SkippedCharacter = Cast<AMyProject1Character>(ReservedTimeSkipCharacter.Get()))
		{
			SkippedCharacter->ApplyFatigueForSkippedMinutes(ReservedTimeSkipMinutes, ReservedTimeSkipIsSleep);
		}

		ReservedTimeSkipMinutes = 0;
		ReservedTimeSkipCharacter.Reset();
		ReservedTimeSkipIsSleep = false;
		return;
	}

	// ワープの予約より先に、フラグ付与だけの予約がないか確認する（RequestFadeThenGrantFlag経由の場合はこちらで完結させる）
	if (!ReservedFlagToGrant.IsNone())
	{
		if (AMyProject1Character* Character = ReservedFlagGrantTarget.Get())
		{
			Character->AddFlag(ReservedFlagToGrant);
		}

		ReservedFlagToGrant = NAME_None;
		ReservedFlagGrantTarget.Reset();
		return;
	}

	// ShowTextDuringFadeのナレーション予約があれば、こちらで完結させる（RequestFadeThenShowNarration用）。
	// 通常のRequestFadeThen〇〇と異なり、ここではFadeInタイマーをまだ動かさない
	// （bWaitingForNarrationCompletionにより、HandleWarpFadeOutCompleteが自動セットを保留する）
	if (UDialogComponent* NarrationComponent = ReservedNarrationComponent.Get())
	{
		ReservedNarrationComponent.Reset();
		bWaitingForNarrationCompletion = true;

		NarrationComponent->BeginFadeNarration();
		return;
	}

	// WallWarpLink経由の予約があれば、こちらで完結させる（RequestFadeThenWallWarp用）
	if (ReservedWallWarpLink.IsValid() && ReservedWallWarpCharacter.IsValid())
	{
		ReservedWallWarpLink->ExecuteWarpNow(ReservedWallWarpCharacter.Get());

		ReservedWallWarpLink.Reset();
		ReservedWallWarpCharacter.Reset();
		return;
	}

	// 予約された情報がなければ何もしない
	if (!ReservedPlayer.IsValid() || ReservedWarpID.IsNone() || !WarpDataTable) return;

	FWarpDestination* WarpData = WarpDataTable->FindRow<FWarpDestination>(ReservedWarpID, TEXT("WarpContext"));
	if (!WarpData) return;

	FName CurrentLevelName = FName(*UGameplayStatics::GetCurrentLevelName(GetWorld()));
	FName TargetLevelName = WarpData->TargetLevelName;

	bool bIsSameLevel = TargetLevelName.IsNone() || (TargetLevelName == CurrentLevelName);

	if (bIsSameLevel)
	{
		// 【パターンA】同じマップの場合：暗転した画面の裏で座標を移動させる
		if (ReservedPlayer.IsValid())
		{
			ReservedPlayer->SetActorTransform(WarpData->DestinationTransform);

			// カメラの向きも合わせる
			if (APlayerController* PC = Cast<APlayerController>(ReservedPlayer->GetController()))
			{
				PC->SetControlRotation(WarpData->DestinationTransform.GetRotation().Rotator());
			}

			// イベント分岐で敗北→同一レベル内の施設へワープしてきた場合、bIsDeadのまま行動不能にならないよう復帰させる
			if (bHasActiveEvent)
			{
				if (AMyProject1Character* MyChar = Cast<AMyProject1Character>(ReservedPlayer.Get()))
				{
					MyChar->Revive(1.0f);
				}
			}
		}
		// 明転と入力復帰は、この関数の呼び出し元であるHandleWarpFadeOutCompleteが
		// WarpFadeInDuration秒後に自動的に処理する（同じマップ/別マップどちらの場合も共通）
	}
	else
	{
		// 【パターンB】別のマップの場合：座標を記憶して、マップを開く
		bHasPendingWarp = true;
		PendingWarpTransform = WarpData->DestinationTransform;

		// クエスト・所持品・装備・ステータス（称号フラグ含む）は、レベル移動でキャラクターが
		// 再生成されると初期値に戻ってしまう。消える前の状態を一時スナップショットとして記憶しておき、
		// 新しいキャラクターのBeginPlay（ApplyPendingCharacterLoad）に読み戻させる。
		if (AMyProject1Character* MyChar = Cast<AMyProject1Character>(ReservedPlayer.Get()))
		{
			if (UMyProject1SaveGame* Snapshot = CapturePlayerStateSnapshot(MyChar))
			{
				PendingLoadSaveGame = Snapshot;
			}
		}

		UGameplayStatics::OpenLevel(GetWorld(), TargetLevelName);
	}

	// 予約情報をリセットしておく
	ReservedWarpID = NAME_None;
	ReservedPlayer.Reset();
}

// ----------------------------------------------------
// 3. レベル移動後の座標適用（既存のまま）[cite: 10]
// ----------------------------------------------------
void UMyProject1GameInstance::ApplyPendingWarp(ACharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !bHasPendingWarp) return;

	PlayerCharacter->SetActorTransform(PendingWarpTransform);

	if (APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController()))
	{
		PC->SetControlRotation(PendingWarpTransform.GetRotation().Rotator());
		PC->SetViewTarget(PlayerCharacter);
	}

	bHasPendingWarp = false;
}

// ----------------------------------------------------
// デバッグワープメニュー等がBP側で一覧UIを組み立てるためのデータ取得
// ----------------------------------------------------
TArray<FWarpDestinationInfo> UMyProject1GameInstance::GetAllWarpDestinations() const
{
	TArray<FWarpDestinationInfo> Result;
	if (!WarpDataTable) return Result;

	for (const FName& RowName : WarpDataTable->GetRowNames())
	{
		const FWarpDestination* Row = WarpDataTable->FindRow<FWarpDestination>(RowName, TEXT("GetAllWarpDestinations"));
		if (!Row) continue;

		FWarpDestinationInfo Info;
		Info.WarpID = RowName;
		Info.DisplayName = Row->DisplayName.IsEmpty() ? FText::FromName(RowName) : Row->DisplayName;
		Result.Add(Info);
	}
	return Result;
}

// ----------------------------------------------------
// セーブ/ロード
// ----------------------------------------------------

const FString UMyProject1GameInstance::AutoSaveSlotName = TEXT("AutoSave");

FString UMyProject1GameInstance::GetManualSaveSlotName(int32 SlotIndex)
{
	// 呼び出し側のミスで範囲外が来ても安全なスロット名になるようクランプする
	SlotIndex = FMath::Clamp(SlotIndex, 1, NumManualSaveSlots);
	return FString::Printf(TEXT("SaveSlot%d"), SlotIndex);
}

UMyProject1SaveGame* UMyProject1GameInstance::CapturePlayerStateSnapshot(AMyProject1Character* Character)
{
	if (!Character) return nullptr;

	UMyProject1SaveGame* SaveObj = Cast<UMyProject1SaveGame>(UGameplayStatics::CreateSaveGameObject(UMyProject1SaveGame::StaticClass()));
	if (!SaveObj) return nullptr;

	// 位置と所属レベル
	SaveObj->PlayerLevelName = FName(*UGameplayStatics::GetCurrentLevelName(GetWorld()));
	SaveObj->PlayerTransform = Character->GetActorTransform();

	// ステータス（称号フラグ=UnlockedFlagsもMyStatsに含まれる）
	SaveObj->PlayerStats = Character->MyStats;

	// 装備
	SaveObj->EquippedItems = Character->CurrentEquippedItems;

	// 所持品
	if (UInventoryComponent* Inv = Character->FindComponentByClass<UInventoryComponent>())
	{
		SaveObj->InventoryContent = Inv->InventoryContent;
		SaveObj->Gil = Inv->Gil;
		SaveObj->ObtainedRareItemIDs = Inv->ObtainedRareItemIDs;
	}

	// クエスト進行
	if (UQuestComponent* Quest = Character->GetQuestComponent())
	{
		SaveObj->ActiveQuests = Quest->ActiveQuests;
		SaveObj->CompletedQuests = Quest->CompletedQuests;
		SaveObj->EverCompletedQuestIDs = Quest->EverCompletedQuestIDs;
	}

	// 傷・タトゥー・ピアス・病気
	if (USkinOverlayComponent* Skin = Character->FindComponentByClass<USkinOverlayComponent>())
	{
		SaveObj->ActiveTattoos = Skin->GetActiveTattoos();
		SaveObj->ActiveScars = Skin->GetActiveScars();
		SaveObj->ActivePiercings = Skin->GetActivePiercings();
		SaveObj->ActiveDiseases = Skin->GetActiveDiseases();
	}

	// 暦・時間（GameInstance常駐データ）
	SaveObj->CurrentTimeInMinutes = CurrentTimeInMinutes;
	SaveObj->CurrentYear = CurrentYear;
	SaveObj->CurrentMonth = CurrentMonth;
	SaveObj->CurrentDay = CurrentDay;
	SaveObj->TotalElapsedDays = TotalElapsedDays;
	SaveObj->CurrentCycleState = CurrentCycleState;

	return SaveObj;
}

bool UMyProject1GameInstance::SaveCurrentGame(const FString& SlotName)
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AMyProject1Character* Character = PC ? Cast<AMyProject1Character>(PC->GetPawn()) : nullptr;
	if (!Character) return false;

	UMyProject1SaveGame* SaveObj = CapturePlayerStateSnapshot(Character);
	if (!SaveObj) return false;

	// スロット一覧UIに出す見出し情報。ゲーム進行の復元には使わないので、ここ（明示セーブ）でのみ埋める。
	SaveObj->SavedAtRealTime = FDateTime::Now();
	SaveObj->PlayerDisplayName = SaveObj->PlayerStats.NPCName.IsEmpty() ? Character->CharacterName : SaveObj->PlayerStats.NPCName;

	return UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, 0);
}

bool UMyProject1GameInstance::AutoSaveGame()
{
	return SaveCurrentGame(AutoSaveSlotName);
}

bool UMyProject1GameInstance::LoadSavedGame(const FString& SlotName)
{
	UMyProject1SaveGame* Loaded = Cast<UMyProject1SaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Loaded) return false;

	// レベル移動後、新しいキャラクターのBeginPlayから読み込まれるまで保持しておく
	PendingLoadSaveGame = Loaded;

	// 暦・時間はGameInstance常駐データなので即座に反映
	CurrentTimeInMinutes = Loaded->CurrentTimeInMinutes;
	CurrentYear = Loaded->CurrentYear;
	CurrentMonth = Loaded->CurrentMonth;
	CurrentDay = Loaded->CurrentDay;
	TotalElapsedDays = Loaded->TotalElapsedDays;
	CurrentCycleState = Loaded->CurrentCycleState;

	// 傷・タトゥー・ピアス・病気の「箱」も先にGameInstance側へ反映しておく。
	// SkinOverlayComponent::BeginPlayがLoadOverlayStateFromGameInstance()で自動的に読みに来る。
	SavedActiveTattoos = Loaded->ActiveTattoos;
	SavedActiveScars = Loaded->ActiveScars;
	SavedActivePiercings = Loaded->ActivePiercings;
	SavedActiveDiseases = Loaded->ActiveDiseases;

	// 既存のワープ着地機構に相乗りして、保存された座標にキャラクターを出現させる
	bHasPendingWarp = true;
	PendingWarpTransform = Loaded->PlayerTransform;

	UGameplayStatics::OpenLevel(GetWorld(), Loaded->PlayerLevelName);
	return true;
}

bool UMyProject1GameInstance::DoesSaveGameExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

FSaveSlotDisplayInfo UMyProject1GameInstance::GetSaveSlotInfo(const FString& SlotName) const
{
	FSaveSlotDisplayInfo Info;
	Info.SlotName = SlotName;
	Info.bIsAutoSave = (SlotName == AutoSaveSlotName);

	// 手動スロット名（"SaveSlot3"等）から番号を取り出す。オートセーブは0のまま。
	if (!Info.bIsAutoSave && SlotName.StartsWith(TEXT("SaveSlot")))
	{
		Info.SlotIndex = FCString::Atoi(*SlotName.RightChop(8)); // "SaveSlot" は8文字
	}

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		Info.bHasData = false;
		return Info;
	}

	const UMyProject1SaveGame* Loaded = Cast<UMyProject1SaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Loaded)
	{
		// ファイルはあるが読めない（バージョン不整合など）。空き扱いにせず「壊れている」と分かる最低限だけ返す。
		Info.bHasData = false;
		return Info;
	}

	Info.bHasData = true;

	// 場所の表示名（対応表で引けなければレベル名そのまま）
	if (const FText* Mapped = LevelDisplayNameMap.Find(Loaded->PlayerLevelName))
	{
		Info.LocationName = *Mapped;
	}
	else
	{
		Info.LocationName = FText::FromName(Loaded->PlayerLevelName);
	}

	Info.PlayerName = Loaded->PlayerDisplayName;
	Info.PlayerLevel = Loaded->PlayerStats.Level;

	if (const UEnum* RankEnum = StaticEnum<EAdventurerRank>())
	{
		Info.RankText = RankEnum->GetDisplayNameTextByValue(static_cast<int64>(Loaded->PlayerStats.AdventurerRank));
	}

	Info.Gil = Loaded->Gil;
	Info.InGameYear = Loaded->CurrentYear;
	Info.InGameMonth = Loaded->CurrentMonth;
	Info.InGameDay = Loaded->CurrentDay;
	Info.TotalElapsedDays = Loaded->TotalElapsedDays;

	// 実時間のセーブ日時。旧セーブなど未設定（Ticks==0）の場合は空文字のままにしておく。
	if (Loaded->SavedAtRealTime.GetTicks() != 0)
	{
		Info.SavedAtText = Loaded->SavedAtRealTime.ToString(TEXT("%Y/%m/%d %H:%M"));
	}

	return Info;
}

TArray<FSaveSlotDisplayInfo> UMyProject1GameInstance::GetAllSaveSlotInfos() const
{
	TArray<FSaveSlotDisplayInfo> Result;
	Result.Reserve(NumManualSaveSlots + 1);

	// 先頭にオートセーブ、続いて手動スロット1〜5
	Result.Add(GetSaveSlotInfo(AutoSaveSlotName));
	for (int32 i = 1; i <= NumManualSaveSlots; ++i)
	{
		Result.Add(GetSaveSlotInfo(GetManualSaveSlotName(i)));
	}

	return Result;
}

bool UMyProject1GameInstance::ApplyPendingCharacterLoad(AMyProject1Character* Character)
{
	if (!Character || !PendingLoadSaveGame) return false;

	UMyProject1SaveGame* Loaded = PendingLoadSaveGame;

	// ステータス
	// ExtraStatDisplayNamesはゲームプレイで変化する値ではなく、Blueprint側で決める固定のログ表示ラベルなので、
	// セーブデータ（古いセーブには存在しない/未設定）で上書きせず、ロード前のキャラクター設定を維持する
	const TMap<FName, FString> PreservedExtraStatDisplayNames = Character->MyStats.ExtraStatDisplayNames;
	Character->MyStats = Loaded->PlayerStats;
	Character->MyStats.ExtraStatDisplayNames = PreservedExtraStatDisplayNames;

	// 所持品
	if (UInventoryComponent* Inv = Character->FindComponentByClass<UInventoryComponent>())
	{
		Inv->InventoryContent = Loaded->InventoryContent;
		Inv->Gil = Loaded->Gil;
		Inv->ObtainedRareItemIDs = Loaded->ObtainedRareItemIDs;
		Inv->OnInventoryUpdated.Broadcast();
	}

	// クエスト進行
	if (UQuestComponent* Quest = Character->GetQuestComponent())
	{
		Quest->ActiveQuests = Loaded->ActiveQuests;
		Quest->CompletedQuests = Loaded->CompletedQuests;
		Quest->EverCompletedQuestIDs = Loaded->EverCompletedQuestIDs;
		Quest->OnQuestUpdated.Broadcast(NAME_None);
	}

	// 装備（見た目・鎖・移動制限も含めて既存のEquipItemロジックで正しく再構築させる）
	if (Character->EquipmentDataTable)
	{
		for (const TPair<EEquipmentSlot, FName>& Pair : Loaded->EquippedItems)
		{
			if (FEquipmentData* Row = Character->EquipmentDataTable->FindRow<FEquipmentData>(Pair.Value, TEXT("LoadGameEquip")))
			{
				Character->EquipItem(Pair.Value, *Row);
			}
		}
	}

	// サイクル状態はTotalElapsedDaysから再計算されるだけなので、明示的に呼んで最新化する
	Character->UpdateCycleState();

	Character->NotifyStatsChanged();

	PendingLoadSaveGame = nullptr;
	return true;
}

void UMyProject1GameInstance::AddLogHistoryEntry(const FString& Message, ELogMessageType InLogType)
{
	FLogHistoryEntry NewEntry;
	NewEntry.Message = Message;
	NewEntry.LogType = InLogType;
	LogHistory.Add(NewEntry);

	// 古いものから削除して上限件数を保つ
	while (LogHistory.Num() > MaxLogHistoryEntries)
	{
		LogHistory.RemoveAt(0);
	}
}

// ----------------------------------------------------
// イベント分岐システム
// ----------------------------------------------------
void UMyProject1GameInstance::StartEvent(FName EventID, ACharacter* PlayerCharacter, AActor* EventContextActor, TSoftObjectPtr<USkeletalMesh> ExtraParticipantMeshOverride, bool bHideContextActorDuringAnimEvent)
{
	if (!PlayerCharacter) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: PlayerCharacter is null")); return; }
	if (!EventDefinitionDataTable) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventDefinitionDataTable is not set on GameInstance")); return; }
	if (EventID.IsNone()) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventID is None")); return; }
	if (bHasActiveEvent) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: bHasActiveEvent is already true (ActiveEventID=%s)"), *ActiveEventID.ToString()); return; }

	FEventDefinition* Definition = EventDefinitionDataTable->FindRow<FEventDefinition>(EventID, TEXT("StartEvent"));
	if (!Definition) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventID '%s' not found in EventDefinitionDataTable"), *EventID.ToString()); return; }
	if (Definition->WarpID.IsNone()) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventID '%s' has no WarpID set"), *EventID.ToString()); return; }

	bHasActiveEvent = true;
	ActiveEventID = EventID;
	ActiveEventPlayer = PlayerCharacter;
	ActiveEventContextActor = EventContextActor;
	ActiveEventExtraMeshOverride = ExtraParticipantMeshOverride;
	bActiveEventHideContextActorDuringAnimEvent = bHideContextActorDuringAnimEvent;
	bAnimEventSequenceStarted = false;

	// ClearCondition=AnimationSequenceの場合、暗転明け後のPlayAnimSequenceEventでEventContextActor（NPC）の
	// 基準値をキャッシュするが、それより前（ワープの暗転待ち中）にAIの通常巡回で動いてしまうと、
	// キャッシュ時点で既に立ち位置がズレてしまう。ここで先んじてAIロジックを止めておく
	// （PlayAnimSequenceEvent側でも同じ処理をするため冪等、再開はResolveActiveEvent/PlayAnimEventStep完了時）
	if (Definition->ClearCondition == EEventClearCondition::AnimationSequence)
	{
		if (ACharacter* ContextCharacter = Cast<ACharacter>(EventContextActor))
		{
			if (AAIController* ContextAI = Cast<AAIController>(ContextCharacter->GetController()))
			{
				ContextAI->StopMovement();
				if (UBrainComponent* Brain = ContextAI->GetBrainComponent())
				{
					Brain->PauseLogic(TEXT("AnimSequenceEvent"));
				}
			}
		}
	}

	// 成立条件に制限時間が絡む場合、ここで自動成立タイマーを仕掛けておく（Interact/AnimationSequenceの成立はこのタイマーを使わない）
	if ((Definition->ClearCondition == EEventClearCondition::TimeElapsed || Definition->ClearCondition == EEventClearCondition::Both)
		&& Definition->TimeLimitSeconds > 0.0f)
	{
		GetTimerManager().SetTimer(ActiveEventTimeLimitTimerHandle, this,
			&UMyProject1GameInstance::HandleActiveEventTimeUp, Definition->TimeLimitSeconds, false);
	}

	// RequiredFlagのチェックは施設への強制送致には意味を持たないためバイパスする
	RequestWarp(Definition->WarpID, PlayerCharacter, true);
}

void UMyProject1GameInstance::HandleActiveEventTimeUp()
{
	ResolveActiveEvent(true);
}

void UMyProject1GameInstance::ResolveActiveEvent(bool bSuccess)
{
	if (!bHasActiveEvent) return;

	GetTimerManager().ClearTimer(ActiveEventTimeLimitTimerHandle);

	// AnimEvent（ClearCondition=AnimationSequence）が全Step完了を待たずに強制終了された場合の保険。
	// PlayAnimEventStep側の後片付けが動かないため、ここで明示的に中断・後片付けする
	AbortCurrentAnimEventStepChain();

	FEventDefinition* Definition = EventDefinitionDataTable
		? EventDefinitionDataTable->FindRow<FEventDefinition>(ActiveEventID, TEXT("ResolveActiveEvent"))
		: nullptr;
	ACharacter* PlayerChar = ActiveEventPlayer.Get();

	if (Definition && PlayerChar)
	{
		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerChar))
		{
			const TArray<FEventAction>& Actions = bSuccess ? Definition->SuccessActions : Definition->FailureActions;
			for (const FEventAction& Action : Actions)
			{
				UGameplayActionLibrary::ExecuteAction(RpgInterface, PlayerChar, nullptr, GetWorld(),
					Action.ActionType, Action.ActionPayload, Action.ItemID, Action.ItemAmount);
				UGameplayActionLibrary::ApplyStatChange(RpgInterface, nullptr,
					Action.StatToChange, Action.StatTargetActor, Action.ExtraStatName, Action.StatChangeAmount);
			}
		}
	}

	const FName ReturnID = Definition ? Definition->ReturnWarpID : NAME_None;

	bHasActiveEvent = false;
	ActiveEventID = NAME_None;
	ActiveEventPlayer.Reset();
	ActiveEventContextActor.Reset();
	ActiveEventExtraMeshOverride.Reset();

	if (!ReturnID.IsNone() && PlayerChar)
	{
		RequestWarp(ReturnID, PlayerChar, true);
	}
}

// ----------------------------------------------------
// アニメーションシーケンス再生（PlayAnimSequenceEvent。イベント分岐システムとは独立。GameInstance.h参照）
// ----------------------------------------------------

// PlayTarget（Player/NPC）に応じた再生対象キャラクターを解決する共通処理。
// FAnimEventStep経由の再生、PlayAnimSequenceRowDirectによる直接再生の両方から使う
static ACharacter* ResolveAnimEventTargetCharacter(EStatTargetActor PlayTarget, const TWeakObjectPtr<ACharacter>& PrimaryCharacter, const TWeakObjectPtr<AActor>& SecondaryContextActor)
{
	if (PlayTarget == EStatTargetActor::NPC)
	{
		return Cast<ACharacter>(SecondaryContextActor.Get());
	}
	return PrimaryCharacter.Get();
}

// AnimSequenceDataTable内でFAnimSequenceEntry::TagsにTagが含まれる行を集め、その中から1つをランダムに選ぶ
// （「パンチ1」「パンチ2」のような同じカテゴリ内の複数バリエーションから抽選するための処理。Montage本体だけでなくLine/bIsPlayerLineも使うため、行そのものを返す）
static const FAnimSequenceEntry* PickRandomAnimSequenceEntryForTag(UDataTable* AnimSequenceDataTable, FName Tag)
{
	if (!AnimSequenceDataTable || Tag.IsNone()) return nullptr;

	TArray<const FAnimSequenceEntry*> Candidates;
	for (const FName& RowName : AnimSequenceDataTable->GetRowNames())
	{
		if (const FAnimSequenceEntry* Entry = AnimSequenceDataTable->FindRow<FAnimSequenceEntry>(RowName, TEXT("PickRandomAnimSequenceEntryForTag")))
		{
			if (Entry->Tags.Contains(Tag) && Entry->Montage)
			{
				Candidates.Add(Entry);
			}
		}
	}

	if (Candidates.Num() == 0) return nullptr;
	return Candidates[FMath::RandHelper(Candidates.Num())];
}

void UMyProject1GameInstance::PlayAnimSequenceEvent(FName AnimEventID, ACharacter* PrimaryCharacter, AActor* SecondaryContextActor, bool bFadeInBeforeStart, TSoftObjectPtr<USkeletalMesh> ExtraParticipantMeshOverride, bool bHideSecondaryContextActorDuringAnim)
{
	if (!PrimaryCharacter || AnimEventID.IsNone() || !AnimEventDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceEvent: invalid arguments or AnimEventDataTable not set (AnimEventID=%s)"), *AnimEventID.ToString());
		OnAnimSequenceEventFinished.Broadcast(false);
		return;
	}

	// PlayAnimSequenceRowDirect（位置調整テスト用の単独再生）がAnimEventPrimaryCharacter等の状態を
	// 共有しているため、テスト再生中に本来のイベントが割り込む場合は先に片付けておく
	if (CurrentAnimSequenceRowDirectMontage.IsValid())
	{
		StopAnimSequenceRowDirect();
	}

	// 前回呼び出しのStepチェーンが全Step完了を待たずに残っている状態で呼ばれた場合（会話の連続トリガー等）、
	// 後片付け（DestroyAnimEventExtraActors）を経ずにAnimEventPrimaryCharacter等を上書きしてしまうと、
	// 前回スポーンしたExtra参加者（AnimEventExtraActors）が新しいMesh・SpawnRelativeLocationを反映せず
	// そのまま使い回されてしまう（Tポーズ・位置ズレの原因になる）。新しいイベントを始める前に必ず打ち切る
	if (CurrentAnimEventStepIndex != INDEX_NONE)
	{
		AbortCurrentAnimEventStepChain();
	}

	FAnimEventDefinition* AnimEvent = AnimEventDataTable->FindRow<FAnimEventDefinition>(AnimEventID, TEXT("PlayAnimSequenceEvent"));
	if (!AnimEvent || AnimEvent->Steps.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceEvent: AnimEventID '%s' not found in AnimEventDataTable, or has no Steps"), *AnimEventID.ToString());
		OnAnimSequenceEventFinished.Broadcast(false);
		return;
	}

	CurrentAnimEventID = AnimEventID;
	AnimEventPrimaryCharacter = PrimaryCharacter;
	AnimEventSecondaryContextActor = SecondaryContextActor;

	// FAnimSequenceEntry::MeshLocationOffset/MeshRotationOffset適用前の基準値をキャッシュしておく。
	// 各ステップはこの基準値+Offsetを都度設定するため、途中で値がズレていかない
	if (USkeletalMeshComponent* PrimaryMesh = PrimaryCharacter->GetMesh())
	{
		AnimEventPrimaryBaseMeshLocation = PrimaryMesh->GetRelativeLocation();
		AnimEventPrimaryBaseMeshRotation = PrimaryMesh->GetRelativeRotation();
	}
	// 再生開始直前のワールドTransformを固定基準として保存する（GetAnimSequenceLockedTransform参照）。
	// AMyProject1Character::Tickがこれを見て毎フレーム同じ位置・向きへ固定し直すため、ターゲット追従回転等の
	// 他のTickロジックが割り込んでも再生中はズレない
	AnimEventPrimaryLockedActorLocation = PrimaryCharacter->GetActorLocation();
	AnimEventPrimaryLockedActorRotation = PrimaryCharacter->GetActorRotation();
	if (ACharacter* SecondaryCharacter = Cast<ACharacter>(SecondaryContextActor))
	{
		// 会話終了直後は、AQuestNPCBase側の「向き直り前の向きへ戻す」TickTurn（ETurnMode::ReturnAfterTalk）が
		// まだ回転中の場合がある。これを止めないまま基準値をキャッシュすると、AnimEvent再生中もNPCのActor向きが
		// 変わり続けてOffsetがズレるため、キャッシュ前に必ず打ち切る
		if (AQuestNPCBase* QuestNPC = Cast<AQuestNPCBase>(SecondaryCharacter))
		{
			QuestNPC->StopReturnTurnImmediately();
		}

		// NPCのBehaviorTree（巡回・待機時の向き変更等）がAnimEvent再生中も動き続けると、
		// Offsetで合わせた位置関係とは無関係にNPC自身のActorが移動・回転してズレていくため、
		// 基準値をキャッシュする前に必ずAIロジックを止める（全ステップ完了時に再開する）
		if (AAIController* SecondaryAI = Cast<AAIController>(SecondaryCharacter->GetController()))
		{
			SecondaryAI->StopMovement();
			if (UBrainComponent* Brain = SecondaryAI->GetBrainComponent())
			{
				Brain->PauseLogic(TEXT("AnimSequenceEvent"));
			}
		}

		if (USkeletalMeshComponent* SecondaryMesh = SecondaryCharacter->GetMesh())
		{
			AnimEventSecondaryBaseMeshLocation = SecondaryMesh->GetRelativeLocation();
			AnimEventSecondaryBaseMeshRotation = SecondaryMesh->GetRelativeRotation();
		}
		AnimEventSecondaryLockedActorLocation = SecondaryCharacter->GetActorLocation();
		AnimEventSecondaryLockedActorRotation = SecondaryCharacter->GetActorRotation();
	}

	// このAnimEventID再生中にスポーンするExtra参加者（FAnimSequenceEntry::ExtraPairings）のメッシュ差し替え設定。
	// GetOrSpawnAnimEventExtraActorがスポーン時に参照し、全ステップ完了・強制終了時にリセットする
	CurrentAnimEventExtraMeshOverride = ExtraParticipantMeshOverride;

	// bHideSecondaryContextActorDuringAnim=trueなら、ExtraPairingsで話しかけた相手と同じ見た目を演じる
	// AAnimEventActorと、フィールドに立っている本体（SecondaryContextActor自身）が重なって「分身」に見えないよう、
	// 再生中だけ本体を非表示にする（再表示はDestroyAnimEventExtraActors側で行う）
	if (bHideSecondaryContextActorDuringAnim)
	{
		if (ACharacter* SecondaryCharacterToHide = Cast<ACharacter>(SecondaryContextActor))
		{
			SecondaryCharacterToHide->SetActorHiddenInGame(true);
			AnimEventHiddenSecondaryNPC = SecondaryCharacterToHide;
		}
	}

	// 再生中はマウスのカメラ操作以外（移動・アクション等）をロックする。専用フラグ（bAnimEventInputLocked）を使うため、
	// 会話終了処理（CloseDialog）等がbIsInputLockedをfalseに戻しても影響を受けない。
	// AMyProject1Character::DoLookはこのフラグを見ないため、カメラ操作だけは引き続き可能
	bAnimEventOverrodeMusic = false;
	if (AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(PrimaryCharacter))
	{
		MyPrimaryCharacter->SetAnimEventInputLocked(true);

		// EventBGMが設定されていればイベント中だけBGMをオーバーライドする（未設定ならフィールド/部屋BGMのまま何もしない）。
		// EnterOverrideMusicは、既に本物のRoomMusicVolume内にいた場合でもその状態を記憶し、
		// 終了時（ExitOverrideMusic）に正しく戻せるようにする
		if (!AnimEvent->EventBGM.IsNull() && MyPrimaryCharacter->MusicComp)
		{
			bAnimEventOverrodeMusic = true;
			MyPrimaryCharacter->MusicComp->EnterOverrideMusic(AnimEvent->EventBGM);
		}
	}

	// 再生中はPrimary/Secondary（NPC）のCapsule同士が、Offsetで近づいた位置関係のまま重なることがあるため、
	// CharacterMovementComponentによる押し出し（Depenetration）で位置がズレないよう、Pawnチャンネルへの
	// 応答だけを一時的にIgnoreにする（Capsule自体のコリジョンは切らないため、地面判定・MovementModeは維持される）。
	// 全ステップ完了時（PlayAnimEventStep）に元の応答へ戻す
	if (UCapsuleComponent* PrimaryCapsule = PrimaryCharacter->GetCapsuleComponent())
	{
		AnimEventPrimaryOriginalPawnResponse = PrimaryCapsule->GetCollisionResponseToChannel(ECC_Pawn);
		PrimaryCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	if (ACharacter* SecondaryCharacterForCollision = Cast<ACharacter>(SecondaryContextActor))
	{
		if (UCapsuleComponent* SecondaryCapsule = SecondaryCharacterForCollision->GetCapsuleComponent())
		{
			AnimEventSecondaryOriginalPawnResponse = SecondaryCapsule->GetCollisionResponseToChannel(ECC_Pawn);
			SecondaryCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
	}

	// アニメーション再生だけが目的で、その間キャラクターが移動する必要はないため、重力・床判定・RootMotionに
	// よる位置ズレ/めり込みを防ぐ目的でCharacterMovementComponent自体を一時停止する（MOVE_None。
	// DisableMovementと同じ標準的な停止方法）。全ステップ完了・強制終了時に元のMovementModeへ戻す
	if (UCharacterMovementComponent* PrimaryMovement = PrimaryCharacter->GetCharacterMovement())
	{
		AnimEventPrimaryOriginalMovementMode = PrimaryMovement->MovementMode;
		AnimEventPrimaryOriginalCustomMovementMode = PrimaryMovement->CustomMovementMode;
		PrimaryMovement->SetMovementMode(MOVE_None);
	}
	if (ACharacter* SecondaryCharacterForMovement = Cast<ACharacter>(SecondaryContextActor))
	{
		if (UCharacterMovementComponent* SecondaryMovement = SecondaryCharacterForMovement->GetCharacterMovement())
		{
			AnimEventSecondaryOriginalMovementMode = SecondaryMovement->MovementMode;
			AnimEventSecondaryOriginalCustomMovementMode = SecondaryMovement->CustomMovementMode;
			SecondaryMovement->SetMovementMode(MOVE_None);
		}
	}

	if (bFadeInBeforeStart)
	{
		// ワープを経由しない直接呼び出し（会話等）用。ステップ切り替えと同じ暗転を開始前にも挟む
		TransitionToAnimEventStep(0);
	}
	else
	{
		PlayAnimEventStep(0);
	}
}

bool UMyProject1GameInstance::IsPlayingAnimSequenceEventFor(const ACharacter* Character) const
{
	return Character && AnimEventPrimaryCharacter.Get() == Character;
}

bool UMyProject1GameInstance::IsAnimEventSecondaryContextActor(const AActor* Actor) const
{
	return Actor && AnimEventSecondaryContextActor.Get() == Actor;
}

bool UMyProject1GameInstance::IsActiveEventPendingAnimationSequence() const
{
	if (!bHasActiveEvent || !EventDefinitionDataTable) return false;

	FEventDefinition* Definition = EventDefinitionDataTable->FindRow<FEventDefinition>(ActiveEventID, TEXT("IsActiveEventPendingAnimationSequence"));
	return Definition && Definition->ClearCondition == EEventClearCondition::AnimationSequence;
}

bool UMyProject1GameInstance::IsActiveEventContextActorPendingAnimationSequence(const AActor* Actor) const
{
	return Actor && ActiveEventContextActor.Get() == Actor && IsActiveEventPendingAnimationSequence();
}

bool UMyProject1GameInstance::GetAnimSequenceLockedTransform(const AActor* Actor, FVector& OutLocation, FRotator& OutRotation) const
{
	if (!Actor) return false;

	if (AnimEventPrimaryCharacter.Get() == Actor)
	{
		OutLocation = AnimEventPrimaryLockedActorLocation;
		OutRotation = AnimEventPrimaryLockedActorRotation;
		return true;
	}
	if (AnimEventSecondaryContextActor.Get() == Actor)
	{
		OutLocation = AnimEventSecondaryLockedActorLocation;
		OutRotation = AnimEventSecondaryLockedActorRotation;
		return true;
	}
	return false;
}

void UMyProject1GameInstance::PlayAnimEventStep(int32 StepIndex)
{
	GetTimerManager().ClearTimer(AnimEventStepDurationTimerHandle);
	GetTimerManager().ClearTimer(AnimEventStepFadeLeadTimerHandle);
	bAnimEventStepLooping = false;

	FAnimEventDefinition* AnimEvent = AnimEventDataTable
		? AnimEventDataTable->FindRow<FAnimEventDefinition>(CurrentAnimEventID, TEXT("PlayAnimEventStep"))
		: nullptr;

	if (!AnimEvent || !AnimEvent->Steps.IsValidIndex(StepIndex))
	{
		// 全ステップ再生完了：入力ロックを解除し、EventBGMでオーバーライドしていれば元のBGMに戻してから状態をクリアする
		if (AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(AnimEventPrimaryCharacter.Get()))
		{
			MyPrimaryCharacter->SetAnimEventInputLocked(false);

			if (bAnimEventOverrodeMusic && MyPrimaryCharacter->MusicComp)
			{
				MyPrimaryCharacter->MusicComp->ExitOverrideMusic();
			}
		}

		// MeshLocationOffset/MeshRotationOffsetで動かした分を、キャッシュしておいた基準値へ戻す
		if (ACharacter* PrimaryCharacter = AnimEventPrimaryCharacter.Get())
		{
			if (USkeletalMeshComponent* PrimaryMesh = PrimaryCharacter->GetMesh())
			{
				PrimaryMesh->SetRelativeLocation(AnimEventPrimaryBaseMeshLocation);
				PrimaryMesh->SetRelativeRotation(AnimEventPrimaryBaseMeshRotation);
			}
			// 再生開始時にIgnoreにしたPawn応答を元に戻す
			if (UCapsuleComponent* PrimaryCapsule = PrimaryCharacter->GetCapsuleComponent())
			{
				PrimaryCapsule->SetCollisionResponseToChannel(ECC_Pawn, AnimEventPrimaryOriginalPawnResponse);
			}
			// 再生開始時にMOVE_Noneで止めたMovementModeを元に戻す
			if (UCharacterMovementComponent* PrimaryMovement = PrimaryCharacter->GetCharacterMovement())
			{
				PrimaryMovement->SetMovementMode(AnimEventPrimaryOriginalMovementMode, AnimEventPrimaryOriginalCustomMovementMode);
			}
		}
		if (ACharacter* SecondaryCharacter = Cast<ACharacter>(AnimEventSecondaryContextActor.Get()))
		{
			if (USkeletalMeshComponent* SecondaryMesh = SecondaryCharacter->GetMesh())
			{
				SecondaryMesh->SetRelativeLocation(AnimEventSecondaryBaseMeshLocation);
				SecondaryMesh->SetRelativeRotation(AnimEventSecondaryBaseMeshRotation);
			}
			if (UCapsuleComponent* SecondaryCapsule = SecondaryCharacter->GetCapsuleComponent())
			{
				SecondaryCapsule->SetCollisionResponseToChannel(ECC_Pawn, AnimEventSecondaryOriginalPawnResponse);
			}
			if (UCharacterMovementComponent* SecondaryMovement = SecondaryCharacter->GetCharacterMovement())
			{
				SecondaryMovement->SetMovementMode(AnimEventSecondaryOriginalMovementMode, AnimEventSecondaryOriginalCustomMovementMode);
			}
			// 再生開始時に止めたNPCのAIロジック（BehaviorTree）を再開する
			if (AAIController* SecondaryAI = Cast<AAIController>(SecondaryCharacter->GetController()))
			{
				if (UBrainComponent* Brain = SecondaryAI->GetBrainComponent())
				{
					Brain->ResumeLogic(TEXT("AnimSequenceEvent"));
				}
			}
		}

		// Extra参加者用にスポーンしたAAnimEventActorを全て破棄する
		DestroyAnimEventExtraActors();

		bAnimEventOverrodeMusic = false;
		CurrentAnimEventExtraMeshOverride.Reset();
		CurrentAnimEventStepIndex = INDEX_NONE;
		CurrentAnimEventID = NAME_None;
		AnimEventPrimaryCharacter.Reset();
		AnimEventSecondaryContextActor.Reset();
		CurrentAnimEventMontage.Reset();
		OnAnimSequenceEventFinished.Broadcast(true);
		return;
	}

	CurrentAnimEventStepIndex = StepIndex;
	const FAnimEventStep& Step = AnimEvent->Steps[StepIndex];

	ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(Step.PlayTarget, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);

	// このステップの再生開始時に、同じTagを持つ候補（パンチ1/パンチ2等）から1回だけ抽選する。
	// bLoop中に終了→再生を繰り返す間は、HandleAnimEventStepMontageEndedがここで選ばれたMontageをそのまま再生し続ける（再抽選しない）
	const FAnimSequenceEntry* SelectedEntry = PickRandomAnimSequenceEntryForTag(AnimSequenceDataTable, Step.Tag);
	UAnimMontage* Montage = SelectedEntry ? SelectedEntry->Montage : nullptr;

	SyncAnimEventEquipmentVisibility(TargetCharacter, SelectedEntry && SelectedEntry->bHideAllEquipmentDuringPlay);

	UAnimInstance* AnimInst = (TargetCharacter && TargetCharacter->GetMesh()) ? TargetCharacter->GetMesh()->GetAnimInstance() : nullptr;

	// このステップで実際に何か再生できたか（メイン対象・Extra参加者のいずれか1体でも再生できればステップは成立する）と、
	// 全参加者の中で最も短いモンタージュの長さ（非ループステップの暗転タイミングを合わせる基準。複数体同時再生時は
	// 最も短いものに合わせて次のステップへ進む仕様。bLoopの場合はStep.Durationが基準のままなので使わない）
	bool bAnyPlayed = false;
	float ShortestMontageLength = -1.0f;

	if (Montage && AnimInst)
	{
		bAnyPlayed = true;
		CurrentAnimEventMontage = Montage;

		// SelectedEntryのMeshLocationOffset/MeshRotationOffsetを、キャッシュしておいた基準値に加算して適用する。
		// PlayTargetがNPC（Secondary）かPlayer（Primary）かで参照する基準値を切り替える
		if (USkeletalMeshComponent* TargetMesh = TargetCharacter->GetMesh())
		{
			const bool bIsSecondaryTarget = (Step.PlayTarget == EStatTargetActor::NPC);
			const FVector& BaseLocation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshLocation : AnimEventPrimaryBaseMeshLocation;
			const FRotator& BaseRotation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshRotation : AnimEventPrimaryBaseMeshRotation;
			TargetMesh->SetRelativeLocation(BaseLocation + SelectedEntry->MeshLocationOffset);
			TargetMesh->SetRelativeRotation(BaseRotation + SelectedEntry->MeshRotationOffset);

			// PropMesh（椅子等）はPlayer側の再生時のみスポーン対象（GameInstance.h参照）
			if (!bIsSecondaryTarget)
			{
				SpawnOrUpdateAnimSequenceProp(*SelectedEntry);
			}
		}

		PlayAnimEventStepMontage(AnimInst, Montage);

		// FAnimSequenceEntry::Soundの再生。前ステップと同じSoundWaveなら再生し直さず継続し、
		// 異なる場合（None⇔設定済みを含む）のみ前のサウンドを止めてから切り替える
		if (SelectedEntry->Sound != CurrentAnimEventSound.Get())
		{
			if (UAudioComponent* PrevAudio = CurrentAnimEventAudioComponent.Get())
			{
				PrevAudio->Stop();
			}
			CurrentAnimEventAudioComponent = nullptr;

			if (SelectedEntry->Sound)
			{
				CurrentAnimEventAudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, SelectedEntry->Sound, TargetCharacter->GetActorLocation());
			}
			CurrentAnimEventSound = SelectedEntry->Sound;
		}

		// セリフが設定されていれば、再生開始と同時にログへ出す（bIsPlayerLine=trueならプレイヤー名付き、falseなら名前なし）
		if (!SelectedEntry->Line.IsEmpty())
		{
			FString LogMsg;
			if (SelectedEntry->bIsPlayerLine)
			{
				AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(AnimEventPrimaryCharacter.Get());
				FString PlayerName = (MyPrimaryCharacter && !MyPrimaryCharacter->MyStats.NPCName.IsEmpty())
					? MyPrimaryCharacter->MyStats.NPCName : TEXT("???");
				LogMsg = FString::Printf(TEXT("%s : %s"), *PlayerName, *SelectedEntry->Line.ToString());
			}
			else
			{
				LogMsg = SelectedEntry->Line.ToString();
			}

			if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(TargetCharacter))
			{
				RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::Dialogue);
			}
		}

		ShortestMontageLength = Montage->GetPlayLength();
	}
	else
	{
		CurrentAnimEventMontage.Reset();
	}

	// Extra参加者（喧嘩の2対1等、メイン対象と同時に別のアニメーションを再生する追加NPC）を1体ずつ再生する。
	// どの追加参加者が出るかは、このステップで実際に抽選で選ばれた行（SelectedEntry）が持つ設定に従う
	// （例："Wave"というTagの中に単体用の行と3人用の行を両方用意しておけば、抽選結果次第で単体・乱闘が切り替わる）
	CurrentAnimEventExtraMontages.Reset();
	if (SelectedEntry)
	{
		for (const FAnimEventPairing& Pairing : SelectedEntry->ExtraPairings)
		{
			const float PairingMontageLength = PlayAnimEventPairing(Pairing);
			if (PairingMontageLength < 0.0f) continue;

			bAnyPlayed = true;
			if (ShortestMontageLength < 0.0f || PairingMontageLength < ShortestMontageLength)
			{
				ShortestMontageLength = PairingMontageLength;
			}
		}
	}

	if (!bAnyPlayed)
	{
		// 再生対象・アセットのいずれも見つからない場合は、このステップを飛ばして次へ進む
		PlayAnimEventStep(StepIndex + 1);
		return;
	}

	if (Step.bLoop && Step.Duration > 0.0f)
	{
		bAnimEventStepLooping = true;

		// Durationいっぱいまでループさせてから暗転を始めると、暗転が完了しきる前にMontageを打ち切ることになり
		// Idleが透けて見えてしまう。暗転にかかる時間（WarpFadeOutDuration）を差し引いた時点で暗転を開始し、
		// その間もループを継続、暗転が完全に終わったタイミング（HandleAnimEventStepTransitionFadeComplete）で
		// 打ち切ることで、トータルの再生時間はDurationのまま、打ち切り自体は暗転で隠せる
		const float LoopFadeLeadTime = FMath::Max(Step.Duration - WarpFadeOutDuration, 0.0f);
		GetTimerManager().SetTimer(AnimEventStepDurationTimerHandle, this,
			&UMyProject1GameInstance::HandleAnimEventStepDurationTimeUp, LoopFadeLeadTime, false);
	}
	else
	{
		// 単発ステップ：全参加者の中で最も短いモンタージュの残り時間がWarpFadeOutDuration秒を切るタイミングで
		// 暗転を開始する。こうすることで、最短のモンタージュが自然終了してIdleへ戻るのとほぼ同時に画面が
		// 完全に暗くなり、暗転が完了しきる前にIdleが透けて見える現象を防ぐ
		const float FadeLeadTime = FMath::Max(ShortestMontageLength - WarpFadeOutDuration, 0.0f);
		GetTimerManager().SetTimer(AnimEventStepFadeLeadTimerHandle, this,
			&UMyProject1GameInstance::HandleAnimEventStepFadeLeadTimeUp, FadeLeadTime, false);
	}
}

// ParticipantIDのAAnimEventActorが未スポーンならAnimEventPrimaryCharacter基準でスポーンする
AAnimEventActor* UMyProject1GameInstance::GetOrSpawnAnimEventExtraActor(const FAnimEventPairing& Pairing)
{
	if (Pairing.ParticipantID.IsNone()) return nullptr;

	ACharacter* PrimaryCharacter = AnimEventPrimaryCharacter.Get();
	if (!PrimaryCharacter) return nullptr;

	// SpawnRelativeLocation/Rotation・Meshは、同じParticipantIDでもStepごとに異なる場合がある
	// （例：Step0は立ち姿勢、Step1は横たわる姿勢など）。既にスポーン済みでも、SpawnOrUpdateAnimSequenceProp
	// と同じ「スポーンor更新」方式で、呼ばれるたびに今回のPairingの内容へ更新し直す
	const FTransform RelativeTransform(Pairing.SpawnRelativeRotation, Pairing.SpawnRelativeLocation);
	const FTransform SpawnTransform = RelativeTransform * PrimaryCharacter->GetActorTransform();

	// CurrentAnimEventExtraMeshOverrideが設定されていれば、DT_AnimSequences側のPairing.Meshより優先する
	// （DT_Dialogs::AnimSequenceNPCMeshOverride経由。同じAnimEventIDを複数種のNPC見た目で使い回すための機能）
	const TSoftObjectPtr<USkeletalMesh>& MeshToUse = !CurrentAnimEventExtraMeshOverride.IsNull() ? CurrentAnimEventExtraMeshOverride : Pairing.Mesh;

	if (AAnimEventActor* Existing = AnimEventExtraActors.FindRef(Pairing.ParticipantID).Get())
	{
		Existing->SetActorTransform(SpawnTransform);
		if (USkeletalMeshComponent* ExistingMesh = Existing->GetMesh())
		{
			if (USkeletalMesh* LoadedMesh = MeshToUse.LoadSynchronous())
			{
				ExistingMesh->SetSkeletalMesh(LoadedMesh);
			}
		}
		return Existing;
	}

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAnimEventActor* NewActor = World->SpawnActor<AAnimEventActor>(AAnimEventActor::StaticClass(), SpawnTransform, SpawnParams);
	if (!NewActor) return nullptr;

	if (USkeletalMeshComponent* NewMesh = NewActor->GetMesh())
	{
		if (USkeletalMesh* LoadedMesh = MeshToUse.LoadSynchronous())
		{
			NewMesh->SetSkeletalMesh(LoadedMesh);
		}
	}

	AnimEventExtraActors.Add(Pairing.ParticipantID, NewActor);
	return NewActor;
}

// 抽選で選ばれたFAnimSequenceEntry::ExtraPairingsの1件分を再生する。再生できればモンタージュの長さを、できなければ負値を返す
float UMyProject1GameInstance::PlayAnimEventPairing(const FAnimEventPairing& Pairing, bool bLoopUntilStopped)
{
	if (Pairing.ParticipantID.IsNone()) return -1.0f;

	AAnimEventActor* ExtraActor = GetOrSpawnAnimEventExtraActor(Pairing);
	if (!ExtraActor) return -1.0f;

	UAnimMontage* Montage = Pairing.Montage;

	USkeletalMeshComponent* TargetMesh = ExtraActor->GetMesh();
	if (!Montage || !TargetMesh) return -1.0f;

	// mapに置いたSkeletalMeshComponentにAnimation Sequenceを直接セットして再生するのと同じ仕組み。
	// PlayAnimationはAnimationMode切替・SingleNodeInstance生成・Slotノード登録までまとめて行うため、
	// ABP（AnimClass）を用意せずにMontageを再生できる
	TargetMesh->PlayAnimation(Montage, false);
	UAnimInstance* AnimInst = TargetMesh->GetAnimInstance();
	if (!AnimInst) return -1.0f;

	CurrentAnimEventExtraMontages.Add(Pairing.ParticipantID, Montage);

	PlayAnimEventStepMontage(AnimInst, Montage);

	if (bLoopUntilStopped)
	{
		// PlayAnimSequenceRowDirect専用：位置調整中はポーズを保つため、Stepシステムのデリゲート
		// （PlayAnimEventStepMontageが直前に設定したもの）を専用のループ処理で上書きする
		FOnMontageBlendingOutStarted LoopDelegate;
		LoopDelegate.BindUFunction(this, FName(TEXT("HandleAnimSequenceRowDirectExtraMontageBlendingOut")));
		AnimInst->Montage_SetBlendingOutDelegate(LoopDelegate, Montage);
	}

	if (!Pairing.Line.IsEmpty())
	{
		FString LogMsg;
		if (Pairing.bIsPlayerLine)
		{
			AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(AnimEventPrimaryCharacter.Get());
			FString PlayerName = (MyPrimaryCharacter && !MyPrimaryCharacter->MyStats.NPCName.IsEmpty())
				? MyPrimaryCharacter->MyStats.NPCName : TEXT("???");
			LogMsg = FString::Printf(TEXT("%s : %s"), *PlayerName, *Pairing.Line.ToString());
		}
		else
		{
			LogMsg = Pairing.Line.ToString();
		}

		// AAnimEventActorはIRpgCharacterInterfaceを実装しない表示専用アクターのため、
		// セリフは常にプレイヤー（AnimEventPrimaryCharacter）のログへ出す
		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(AnimEventPrimaryCharacter.Get()))
		{
			RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::Dialogue);
		}
	}

	return Montage->GetPlayLength();
}

// AnimEventExtraActorsに残っている全Extra参加者と、CurrentAnimSequencePropActorを破棄してクリアする
void UMyProject1GameInstance::DestroyAnimEventExtraActors()
{
	// FAnimSequenceEntry::Soundで再生中のサウンドを停止する（全Step完了・強制終了どちらの後片付けからも呼ばれる）
	if (UAudioComponent* CurrentAudio = CurrentAnimEventAudioComponent.Get())
	{
		CurrentAudio->Stop();
	}
	CurrentAnimEventAudioComponent = nullptr;
	CurrentAnimEventSound = nullptr;

	for (const TPair<FName, TWeakObjectPtr<AAnimEventActor>>& Pair : AnimEventExtraActors)
	{
		if (AAnimEventActor* ExtraActor = Pair.Value.Get())
		{
			ExtraActor->Destroy();
		}
	}
	AnimEventExtraActors.Reset();
	CurrentAnimEventExtraMontages.Reset();

	if (AStaticMeshActor* PropActor = CurrentAnimSequencePropActor.Get())
	{
		PropActor->Destroy();
	}
	CurrentAnimSequencePropActor.Reset();

	// PlayAnimSequenceEventがbHideSecondaryContextActorDuringAnim=trueで非表示にしたSecondaryContextActorを再表示する
	if (ACharacter* HiddenNPC = AnimEventHiddenSecondaryNPC.Get())
	{
		HiddenNPC->SetActorHiddenInGame(false);
	}
	AnimEventHiddenSecondaryNPC.Reset();

	// SyncAnimEventEquipmentVisibilityがFAnimSequenceEntry::bHideAllEquipmentDuringPlay=trueで非表示にした装備を再表示する
	if (ACharacter* HiddenEquipChar = AnimEventHiddenEquipmentCharacter.Get())
	{
		if (AMyProject1Character* MyHiddenEquipChar = Cast<AMyProject1Character>(HiddenEquipChar))
		{
			MyHiddenEquipChar->SetAllEquipmentComponentsVisible(true);
		}
	}
	AnimEventHiddenEquipmentCharacter.Reset();
}

// FAnimSequenceEntry::bHideAllEquipmentDuringPlayに従って、TargetCharacterの装備表示状態を同期する（GameInstance.h参照）
void UMyProject1GameInstance::SyncAnimEventEquipmentVisibility(ACharacter* TargetCharacter, bool bWantHidden)
{
	ACharacter* CurrentlyHidden = AnimEventHiddenEquipmentCharacter.Get();

	// 対象が切り替わった、または非表示不要になった場合は、前回非表示にしたキャラクターを先に再表示する
	if (CurrentlyHidden && (CurrentlyHidden != TargetCharacter || !bWantHidden))
	{
		if (AMyProject1Character* MyCurrentlyHidden = Cast<AMyProject1Character>(CurrentlyHidden))
		{
			MyCurrentlyHidden->SetAllEquipmentComponentsVisible(true);
		}
		AnimEventHiddenEquipmentCharacter.Reset();
	}

	if (bWantHidden && TargetCharacter && AnimEventHiddenEquipmentCharacter.Get() != TargetCharacter)
	{
		if (AMyProject1Character* MyTargetCharacter = Cast<AMyProject1Character>(TargetCharacter))
		{
			MyTargetCharacter->SetAllEquipmentComponentsVisible(false);
			AnimEventHiddenEquipmentCharacter = TargetCharacter;
		}
	}
}

// 進行中のAnimSequenceEvent（PlayAnimEventStepのStepチェーン）を、全Step完了を待たずに強制的に中断する。
// ResolveActiveEventの強制終了パスと、PlayAnimSequenceEventの再入防止（前回のStepチェーンが終わっていないまま
// 新しいイベントが呼ばれた場合、古いExtra参加者が新しいMesh・位置設定を受けずに使い回されてしまうのを防ぐ）から呼ぶ
void UMyProject1GameInstance::AbortCurrentAnimEventStepChain()
{
	GetTimerManager().ClearTimer(AnimEventStepDurationTimerHandle);
	GetTimerManager().ClearTimer(AnimEventStepTransitionTimerHandle);
	CurrentAnimEventStepIndex = INDEX_NONE;
	bAnimEventStepLooping = false;
	bAnimEventSequenceStarted = false;
	CurrentAnimEventID = NAME_None;
	PendingAnimEventNextStepIndex = INDEX_NONE;
	CurrentAnimEventExtraMeshOverride.Reset();

	// PlayAnimEventStep側の後片付けが動かないため、止めた入力ロック・NPCのAIロジックをここで明示的に戻す。
	// あわせて、Mesh位置Offset・Capsuleの一時的なコリジョン応答・MovementModeも正常完了時と同じ内容へ戻す
	// （ここを戻さないまま次のPlayAnimSequenceEventが基準位置を再キャッシュすると、Offset分が基準値に
	// 混入して位置がズレ続ける原因になるため）
	if (ACharacter* PrimaryCharacter = AnimEventPrimaryCharacter.Get())
	{
		if (AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(PrimaryCharacter))
		{
			MyPrimaryCharacter->SetAnimEventInputLocked(false);

			// EventBGMでオーバーライドしていた場合、正常完了時（PlayAnimEventStep）と同じくBGM状態を戻す
			if (bAnimEventOverrodeMusic && MyPrimaryCharacter->MusicComp)
			{
				MyPrimaryCharacter->MusicComp->ExitOverrideMusic();
			}
		}
		if (USkeletalMeshComponent* PrimaryMesh = PrimaryCharacter->GetMesh())
		{
			PrimaryMesh->SetRelativeLocation(AnimEventPrimaryBaseMeshLocation);
			PrimaryMesh->SetRelativeRotation(AnimEventPrimaryBaseMeshRotation);
		}
		if (UCapsuleComponent* PrimaryCapsule = PrimaryCharacter->GetCapsuleComponent())
		{
			PrimaryCapsule->SetCollisionResponseToChannel(ECC_Pawn, AnimEventPrimaryOriginalPawnResponse);
		}
		if (UCharacterMovementComponent* PrimaryMovement = PrimaryCharacter->GetCharacterMovement())
		{
			PrimaryMovement->SetMovementMode(AnimEventPrimaryOriginalMovementMode, AnimEventPrimaryOriginalCustomMovementMode);
		}
	}
	if (ACharacter* SecondaryCharacter = Cast<ACharacter>(AnimEventSecondaryContextActor.Get()))
	{
		if (AAIController* SecondaryAI = Cast<AAIController>(SecondaryCharacter->GetController()))
		{
			if (UBrainComponent* Brain = SecondaryAI->GetBrainComponent())
			{
				Brain->ResumeLogic(TEXT("AnimSequenceEvent"));
			}
		}
		if (USkeletalMeshComponent* SecondaryMesh = SecondaryCharacter->GetMesh())
		{
			SecondaryMesh->SetRelativeLocation(AnimEventSecondaryBaseMeshLocation);
			SecondaryMesh->SetRelativeRotation(AnimEventSecondaryBaseMeshRotation);
		}
		if (UCapsuleComponent* SecondaryCapsule = SecondaryCharacter->GetCapsuleComponent())
		{
			SecondaryCapsule->SetCollisionResponseToChannel(ECC_Pawn, AnimEventSecondaryOriginalPawnResponse);
		}
		if (UCharacterMovementComponent* SecondaryMovement = SecondaryCharacter->GetCharacterMovement())
		{
			SecondaryMovement->SetMovementMode(AnimEventSecondaryOriginalMovementMode, AnimEventSecondaryOriginalCustomMovementMode);
		}
	}

	AnimEventPrimaryCharacter.Reset();
	AnimEventSecondaryContextActor.Reset();
	CurrentAnimEventMontage.Reset();

	DestroyAnimEventExtraActors();
}

// Entry.PropMeshが設定されていれば、AnimEventPrimaryCharacter（Player）基準のPropRelativeLocation/Rotationへ
// CurrentAnimSequencePropActorをスポーン（未スポーンの場合）またはメッシュ差し替え・位置更新する。
// PropMesh未設定なら何もしない（Player再生時のみ呼ばれる想定。NPC側では呼ばない）
void UMyProject1GameInstance::SpawnOrUpdateAnimSequenceProp(const FAnimSequenceEntry& Entry)
{
	if (Entry.PropMesh.IsNull())
	{
		// 前のStepでPropMesh付きの行がスポーンしたPropが、今回のStepではPropMesh未設定で不要になった場合、
		// 消し忘れて次のStep（さらにはイベント完了まで）残り続けないよう、ここで破棄する
		if (AStaticMeshActor* StaleProp = CurrentAnimSequencePropActor.Get())
		{
			StaleProp->Destroy();
			CurrentAnimSequencePropActor.Reset();
		}
		return;
	}

	ACharacter* PrimaryCharacter = AnimEventPrimaryCharacter.Get();
	UWorld* World = GetWorld();
	if (!PrimaryCharacter || !World) return;

	UStaticMesh* LoadedMesh = Entry.PropMesh.LoadSynchronous();
	if (!LoadedMesh) return;

	const FTransform RelativeTransform(Entry.PropRelativeRotation, Entry.PropRelativeLocation);
	const FTransform SpawnTransform = RelativeTransform * PrimaryCharacter->GetActorTransform();

	AStaticMeshActor* PropActor = CurrentAnimSequencePropActor.Get();
	if (!PropActor)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		PropActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnTransform, SpawnParams);
		if (!PropActor) return;

		PropActor->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MeshComp = PropActor->GetStaticMeshComponent())
		{
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		CurrentAnimSequencePropActor = PropActor;
	}
	else
	{
		PropActor->SetActorTransform(SpawnTransform);
	}

	if (UStaticMeshComponent* MeshComp = PropActor->GetStaticMeshComponent())
	{
		MeshComp->SetStaticMesh(LoadedMesh);
	}
}

// ----------------------------------------------------
// DT_AnimSequencesの1行を直接再生するテスト用機能（位置調整確認用。GameInstance.h参照）
// ----------------------------------------------------

void UMyProject1GameInstance::PlayAnimSequenceRowDirect(FName RowName, ACharacter* PrimaryCharacter, AActor* SecondaryContextActor, EStatTargetActor PlayTarget)
{
	if (!PrimaryCharacter || RowName.IsNone() || !AnimSequenceDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceRowDirect: invalid arguments or AnimSequenceDataTable not set (RowName=%s)"), *RowName.ToString());
		return;
	}

	// DT_AnimEventsのStepが進行中の間は、AnimEventPrimaryCharacter等の状態を奪い合うため使用できない
	if (CurrentAnimEventStepIndex != INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceRowDirect: an AnimEvent sequence is currently playing, ignored (RowName=%s)"), *RowName.ToString());
		return;
	}

	const FAnimSequenceEntry* Entry = AnimSequenceDataTable->FindRow<FAnimSequenceEntry>(RowName, TEXT("PlayAnimSequenceRowDirect"));
	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceRowDirect: RowName '%s' not found in AnimSequenceDataTable"), *RowName.ToString());
		return;
	}

	// 前回のテスト再生の残骸（Extra参加者・位置オフセット・ループ再生中のモンタージュ）が残っていれば先に片付ける
	StopAnimSequenceRowDirect();

	AnimEventPrimaryCharacter = PrimaryCharacter;
	AnimEventSecondaryContextActor = SecondaryContextActor;
	CurrentAnimSequenceRowDirectPlayTarget = PlayTarget;

	// FAnimSequenceEntry::MeshLocationOffset/MeshRotationOffset適用前の基準値をキャッシュしておく（PlayAnimSequenceEventと同じ方式）
	if (USkeletalMeshComponent* PrimaryMesh = PrimaryCharacter->GetMesh())
	{
		AnimEventPrimaryBaseMeshLocation = PrimaryMesh->GetRelativeLocation();
		AnimEventPrimaryBaseMeshRotation = PrimaryMesh->GetRelativeRotation();
	}
	// PlayAnimSequenceEventと同じく、再生開始直前のワールドTransformを固定基準として保存する
	AnimEventPrimaryLockedActorLocation = PrimaryCharacter->GetActorLocation();
	AnimEventPrimaryLockedActorRotation = PrimaryCharacter->GetActorRotation();
	if (ACharacter* SecondaryCharacter = Cast<ACharacter>(SecondaryContextActor))
	{
		if (AQuestNPCBase* QuestNPC = Cast<AQuestNPCBase>(SecondaryCharacter))
		{
			QuestNPC->StopReturnTurnImmediately();
		}

		if (AAIController* SecondaryAI = Cast<AAIController>(SecondaryCharacter->GetController()))
		{
			SecondaryAI->StopMovement();
			if (UBrainComponent* Brain = SecondaryAI->GetBrainComponent())
			{
				Brain->PauseLogic(TEXT("AnimSequenceRowDirect"));
			}
		}

		if (USkeletalMeshComponent* SecondaryMesh = SecondaryCharacter->GetMesh())
		{
			AnimEventSecondaryBaseMeshLocation = SecondaryMesh->GetRelativeLocation();
			AnimEventSecondaryBaseMeshRotation = SecondaryMesh->GetRelativeRotation();
		}
		AnimEventSecondaryLockedActorLocation = SecondaryCharacter->GetActorLocation();
		AnimEventSecondaryLockedActorRotation = SecondaryCharacter->GetActorRotation();
	}

	// PlayAnimSequenceEventと同じ理由（重力・床判定・RootMotionによる位置ズレ/めり込み防止）で、
	// テスト再生中もCharacterMovementComponentを一時停止する（MOVE_None）。StopAnimSequenceRowDirectで元へ戻す
	if (UCharacterMovementComponent* PrimaryMovement = PrimaryCharacter->GetCharacterMovement())
	{
		AnimEventPrimaryOriginalMovementMode = PrimaryMovement->MovementMode;
		AnimEventPrimaryOriginalCustomMovementMode = PrimaryMovement->CustomMovementMode;
		PrimaryMovement->SetMovementMode(MOVE_None);
	}
	if (ACharacter* SecondaryCharacterForMovement = Cast<ACharacter>(SecondaryContextActor))
	{
		if (UCharacterMovementComponent* SecondaryMovement = SecondaryCharacterForMovement->GetCharacterMovement())
		{
			AnimEventSecondaryOriginalMovementMode = SecondaryMovement->MovementMode;
			AnimEventSecondaryOriginalCustomMovementMode = SecondaryMovement->CustomMovementMode;
			SecondaryMovement->SetMovementMode(MOVE_None);
		}
	}

	bool bAnyPlayed = false;

	const bool bIsSecondaryTarget = (PlayTarget == EStatTargetActor::NPC);
	ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(PlayTarget, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);
	USkeletalMeshComponent* TargetMesh = TargetCharacter ? TargetCharacter->GetMesh() : nullptr;
	UAnimInstance* AnimInst = TargetMesh ? TargetMesh->GetAnimInstance() : nullptr;

	SyncAnimEventEquipmentVisibility(TargetCharacter, Entry->bHideAllEquipmentDuringPlay);

	if (Entry->Montage && TargetMesh && AnimInst)
	{
		bAnyPlayed = true;

		// 位置調整中はNudgeAnimSequenceRowDirectOffsetがこの値を書き換えていく（初期値はDT側の設定値）
		CurrentAnimSequenceRowDirectLocationOffset = Entry->MeshLocationOffset;
		CurrentAnimSequenceRowDirectRotationOffset = Entry->MeshRotationOffset;

		const FVector& BaseLocation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshLocation : AnimEventPrimaryBaseMeshLocation;
		const FRotator& BaseRotation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshRotation : AnimEventPrimaryBaseMeshRotation;
		TargetMesh->SetRelativeLocation(BaseLocation + CurrentAnimSequenceRowDirectLocationOffset);
		TargetMesh->SetRelativeRotation(BaseRotation + CurrentAnimSequenceRowDirectRotationOffset);

		// PropMesh（椅子等）はPlayer側の再生時のみスポーン対象（GameInstance.h参照）。
		// デバッグメニュー（PlayAnimSequenceRowDirect経由）でもここで一緒にプレビューできる
		if (!bIsSecondaryTarget)
		{
			SpawnOrUpdateAnimSequenceProp(*Entry);
		}

		CurrentAnimSequenceRowDirectMontage = Entry->Montage;
		AnimInst->Montage_Play(Entry->Montage);

		// このMontage再生開始と同時に1回だけ再生するサウンド（FAnimSequenceEntry::Sound。未設定なら何もしない。
		// 位置調整用の無限ループ再生（下記）では継ぎ目ごとに再生し直さない＝ここで最初の1回のみ鳴らす）
		if (Entry->Sound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, Entry->Sound, TargetCharacter->GetActorLocation());
		}

		// 位置調整のため、明示的に止める（StopAnimSequenceRowDirect）までループし続ける
		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindUFunction(this, FName(TEXT("HandleAnimSequenceRowDirectMontageBlendingOut")));
		AnimInst->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Entry->Montage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceRowDirect: no Montage on row, or target character/mesh not available (RowName=%s, PlayTarget=%s)"),
			*RowName.ToString(), bIsSecondaryTarget ? TEXT("NPC") : TEXT("Player"));
	}

	// ExtraPairingsも通常のイベント再生と同じ経路（スポーン→オフセット適用→モンタージュ再生）で同時に再生する。
	// メイン対象と同じく、明示的に止める（StopAnimSequenceRowDirect）までループし続ける
	// （Nudgeの対象は引き続きメイン対象のみ。ExtraPairing側のオフセット自体はDT_AnimSequences側の設定値のまま）
	for (const FAnimEventPairing& Pairing : Entry->ExtraPairings)
	{
		if (PlayAnimEventPairing(Pairing, /*bLoopUntilStopped=*/true) >= 0.0f)
		{
			bAnyPlayed = true;
		}
	}

	if (!bAnyPlayed)
	{
		// 何も再生できなかった場合はその場で後片付けする（AnimEventPrimaryCharacter等を残さない）
		StopAnimSequenceRowDirect();
	}
}

void UMyProject1GameInstance::NudgeAnimSequenceRowDirectOffset(FVector LocationDelta, FRotator RotationDelta)
{
	if (!CurrentAnimSequenceRowDirectMontage.IsValid()) return;

	ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(CurrentAnimSequenceRowDirectPlayTarget, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);
	USkeletalMeshComponent* TargetMesh = TargetCharacter ? TargetCharacter->GetMesh() : nullptr;
	if (!TargetMesh) return;

	CurrentAnimSequenceRowDirectLocationOffset += LocationDelta;
	CurrentAnimSequenceRowDirectRotationOffset += RotationDelta;

	const bool bIsSecondaryTarget = (CurrentAnimSequenceRowDirectPlayTarget == EStatTargetActor::NPC);
	const FVector& BaseLocation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshLocation : AnimEventPrimaryBaseMeshLocation;
	const FRotator& BaseRotation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshRotation : AnimEventPrimaryBaseMeshRotation;
	TargetMesh->SetRelativeLocation(BaseLocation + CurrentAnimSequenceRowDirectLocationOffset);
	TargetMesh->SetRelativeRotation(BaseRotation + CurrentAnimSequenceRowDirectRotationOffset);

	// DTへ書き戻す数値をそのまま読み取れるよう、現在値を画面に表示しておく
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(9000, 5.0f, FColor::Yellow, FString::Printf(
			TEXT("MeshLocationOffset=%s\nMeshRotationOffset=%s"),
			*CurrentAnimSequenceRowDirectLocationOffset.ToString(), *CurrentAnimSequenceRowDirectRotationOffset.ToString()));
	}
}

// PlayAnimSequenceRowDirectで再生中のメイン対象モンタージュのBlendingOutデリゲート（GameInstance.h参照）
void UMyProject1GameInstance::HandleAnimSequenceRowDirectMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;
	if (Montage != CurrentAnimSequenceRowDirectMontage.Get()) return;

	ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(CurrentAnimSequenceRowDirectPlayTarget, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);
	UAnimInstance* AnimInst = (TargetCharacter && TargetCharacter->GetMesh()) ? TargetCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInst) return;

	// ブレンドアウトが完了しきる前（まだウェイトが高いうち）に同じモンタージュを再生し直し、ポーズを保つ
	AnimInst->Montage_Play(Montage);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUFunction(this, FName(TEXT("HandleAnimSequenceRowDirectMontageBlendingOut")));
	AnimInst->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);
}

// PlayAnimEventPairing(bLoopUntilStopped=true)で再生したExtra参加者のモンタージュのBlendingOutデリゲート
// （HandleAnimSequenceRowDirectMontageBlendingOutのExtra参加者版）
void UMyProject1GameInstance::HandleAnimSequenceRowDirectExtraMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted) return;

	// StopAnimSequenceRowDirect（DestroyAnimEventExtraActors）で既に破棄済みならnullptrが返るため、
	// その場合は何もせず自然にループが止まる
	UAnimInstance* AnimInst = ResolveAnimInstanceForTrackedMontage(Montage);
	if (!AnimInst) return;

	AnimInst->Montage_Play(Montage);

	FOnMontageBlendingOutStarted LoopDelegate;
	LoopDelegate.BindUFunction(this, FName(TEXT("HandleAnimSequenceRowDirectExtraMontageBlendingOut")));
	AnimInst->Montage_SetBlendingOutDelegate(LoopDelegate, Montage);
}

// PlayAnimSequenceRowDirectで動かした位置を基準値へ戻し、ループ再生中のモンタージュを止め、
// スポーンしたExtra参加者を破棄する
void UMyProject1GameInstance::StopAnimSequenceRowDirect()
{
	if (ACharacter* PrimaryCharacter = AnimEventPrimaryCharacter.Get())
	{
		if (USkeletalMeshComponent* PrimaryMesh = PrimaryCharacter->GetMesh())
		{
			PrimaryMesh->SetRelativeLocation(AnimEventPrimaryBaseMeshLocation);
			PrimaryMesh->SetRelativeRotation(AnimEventPrimaryBaseMeshRotation);
		}
		// テスト再生開始時にMOVE_Noneで止めたMovementModeを元に戻す
		if (UCharacterMovementComponent* PrimaryMovement = PrimaryCharacter->GetCharacterMovement())
		{
			PrimaryMovement->SetMovementMode(AnimEventPrimaryOriginalMovementMode, AnimEventPrimaryOriginalCustomMovementMode);
		}
	}
	if (ACharacter* SecondaryCharacter = Cast<ACharacter>(AnimEventSecondaryContextActor.Get()))
	{
		if (USkeletalMeshComponent* SecondaryMesh = SecondaryCharacter->GetMesh())
		{
			SecondaryMesh->SetRelativeLocation(AnimEventSecondaryBaseMeshLocation);
			SecondaryMesh->SetRelativeRotation(AnimEventSecondaryBaseMeshRotation);
		}
		if (UCharacterMovementComponent* SecondaryMovement = SecondaryCharacter->GetCharacterMovement())
		{
			SecondaryMovement->SetMovementMode(AnimEventSecondaryOriginalMovementMode, AnimEventSecondaryOriginalCustomMovementMode);
		}
		// 再生開始時に止めたNPCのAIロジック（BehaviorTree）を再開する
		if (AAIController* SecondaryAI = Cast<AAIController>(SecondaryCharacter->GetController()))
		{
			if (UBrainComponent* Brain = SecondaryAI->GetBrainComponent())
			{
				Brain->ResumeLogic(TEXT("AnimSequenceRowDirect"));
			}
		}
	}

	// ループ再生中のモンタージュを明示的に止める（bInterrupted=trueとなり、BlendingOutハンドラが再生し直さなくなる）
	if (UAnimMontage* Montage = CurrentAnimSequenceRowDirectMontage.Get())
	{
		ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(CurrentAnimSequenceRowDirectPlayTarget, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);
		if (UAnimInstance* AnimInst = (TargetCharacter && TargetCharacter->GetMesh()) ? TargetCharacter->GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInst->Montage_Stop(0.1f, Montage);
		}
	}

	DestroyAnimEventExtraActors();

	CurrentAnimSequenceRowDirectMontage.Reset();
	AnimEventPrimaryCharacter.Reset();
	AnimEventSecondaryContextActor.Reset();
}

// DT_AnimSequencesの全行を、UI表示用の軽量データ一覧として取得する（GetAllWarpDestinationsと同じパターン）
TArray<FAnimSequenceRowInfo> UMyProject1GameInstance::GetAllAnimSequenceRows() const
{
	TArray<FAnimSequenceRowInfo> Result;
	if (!AnimSequenceDataTable) return Result;

	for (const FName& RowName : AnimSequenceDataTable->GetRowNames())
	{
		const FAnimSequenceEntry* Row = AnimSequenceDataTable->FindRow<FAnimSequenceEntry>(RowName, TEXT("GetAllAnimSequenceRows"));
		if (!Row) continue;

		FAnimSequenceRowInfo Info;
		Info.RowName = RowName;
		Info.Tags = Row->Tags;
		Result.Add(Info);
	}
	return Result;
}

// Montageがメイン参加者・Extra参加者のいずれかで現在再生中として記録されているかを判定する
bool UMyProject1GameInstance::IsTrackedAnimEventMontage(UAnimMontage* Montage) const
{
	if (!Montage) return false;
	if (Montage == CurrentAnimEventMontage.Get()) return true;

	for (const TPair<FName, TWeakObjectPtr<UAnimMontage>>& Pair : CurrentAnimEventExtraMontages)
	{
		if (Pair.Value.Get() == Montage) return true;
	}
	return false;
}

// IsTrackedAnimEventMontageで一致したMontageについて、それを再生しているAnimInstanceを解決する
UAnimInstance* UMyProject1GameInstance::ResolveAnimInstanceForTrackedMontage(UAnimMontage* Montage) const
{
	if (!Montage) return nullptr;

	if (Montage == CurrentAnimEventMontage.Get())
	{
		FAnimEventDefinition* AnimEvent = AnimEventDataTable
			? AnimEventDataTable->FindRow<FAnimEventDefinition>(CurrentAnimEventID, TEXT("ResolveAnimInstanceForTrackedMontage"))
			: nullptr;
		if (!AnimEvent || !AnimEvent->Steps.IsValidIndex(CurrentAnimEventStepIndex)) return nullptr;

		const FAnimEventStep& Step = AnimEvent->Steps[CurrentAnimEventStepIndex];
		ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(Step.PlayTarget, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);
		return (TargetCharacter && TargetCharacter->GetMesh()) ? TargetCharacter->GetMesh()->GetAnimInstance() : nullptr;
	}

	for (const TPair<FName, TWeakObjectPtr<UAnimMontage>>& Pair : CurrentAnimEventExtraMontages)
	{
		if (Pair.Value.Get() != Montage) continue;

		AAnimEventActor* ExtraActor = AnimEventExtraActors.FindRef(Pair.Key).Get();
		return (ExtraActor && ExtraActor->GetMesh()) ? ExtraActor->GetMesh()->GetAnimInstance() : nullptr;
	}

	return nullptr;
}

void UMyProject1GameInstance::HandleAnimEventStepDurationTimeUp()
{
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;

	// ここではまだモンタージュを止めない。次のステップへの暗転を開始するだけにして、
	// 実際の打ち切りは暗転が完全に終わった後（HandleAnimEventStepTransitionFadeComplete）まで遅らせる。
	// こうしないと、暗転が完了しきる前にIdleへ戻る瞬間が透けて見えてしまう
	TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
}

void UMyProject1GameInstance::HandleAnimEventStepMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;

	// Montage_Stopで打ち切った古いステップのモンタージュから遅延して届く終了通知は、
	// 既に次のステップへ進んだ後の状態（bAnimEventStepLooping・タイマー）と誤って結びついてしまうため、
	// 「今のステップで実際に再生している（メイン・Extraいずれかの）モンタージュ」と一致しないものは無視する
	if (!IsTrackedAnimEventMontage(Montage)) return;

	if (bAnimEventStepLooping)
	{
		// ループの継ぎ目の再生し直しはHandleAnimEventStepMontageBlendingOut（ブレンドアウト開始時点）が
		// 既に行っている。ここで発火するEndは、その時点で再生し直した新しいインスタンスに対して
		// 役目を終えた「古いインスタンス」がブレンドアウトを完了した通知にすぎないため、何もしない
		return;
	}

	// 通常はHandleAnimEventStepMontageBlendingOut（ブレンドアウト開始時点）が既に次のステップへの
	// 暗転を始めている（PendingAnimEventNextStepIndexが立っている）。ここに来るのは、何らかの理由で
	// BlendingOut側が発火しなかった場合の保険
	if (PendingAnimEventNextStepIndex == INDEX_NONE)
	{
		TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
	}
}

void UMyProject1GameInstance::HandleAnimEventStepMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// bInterrupted=trueはDuration経過（HandleAnimEventStepDurationTimeUp）等による明示的なMontage_Stop由来。
	// ループ・暗転のどちらも、その打ち切り処理側が既に次のステップへ進めるので、ここでは何もしない
	if (bInterrupted) return;
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;
	if (!IsTrackedAnimEventMontage(Montage)) return;

	if (bAnimEventStepLooping)
	{
		// bAnimEventStepLoopingがtrueの間は、Duration用タイマーが既に発火済み（＝暗転中）でも
		// 同じモンタージュを再生し直してループを継続する（メイン・Extra問わず、参加者ごとに独立して継ぎ目を処理する）。
		// 実際に打ち切るのは暗転が完全に終わったタイミング（HandleAnimEventStepTransitionFadeComplete、
		// そこでbAnimEventStepLoopingをfalseにする）なので、この関数がそれより後に呼ばれることはない
		if (UAnimInstance* AnimInst = ResolveAnimInstanceForTrackedMontage(Montage))
		{
			// ブレンドアウトが完了しきる前（まだウェイトが高いうち）に次の再生を仕込むことで、
			// 完全にIdleへ戻ってから再生し直す場合よりもループの継ぎ目を目立たなくする
			// （新旧2つのMontageインスタンスが同時に存在し、クロスフェードする形になる）
			PlayAnimEventStepMontage(AnimInst, Montage);
		}
		return;
	}

	// ループなしステップ：ブレンドアウトが完了してIdleへ戻りきる前（HandleAnimEventStepMontageEndedの
	// 発火より早いタイミング）で暗転を始めることで、暗転が始まる前にIdleへ一瞬戻って見える現象を防ぐ
	if (PendingAnimEventNextStepIndex == INDEX_NONE)
	{
		TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
	}
}

void UMyProject1GameInstance::PlayAnimEventStepMontage(UAnimInstance* AnimInst, UAnimMontage* Montage)
{
	AnimInst->Montage_Play(Montage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUFunction(this, FName(TEXT("HandleAnimEventStepMontageEnded")));
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUFunction(this, FName(TEXT("HandleAnimEventStepMontageBlendingOut")));
	AnimInst->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);
}

void UMyProject1GameInstance::HandleAnimEventStepFadeLeadTimeUp()
{
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;
	if (bAnimEventStepLooping) return; // ループ中はDurationタイマー側が進行を管理するため対象外
	if (PendingAnimEventNextStepIndex != INDEX_NONE) return; // 既に他経路で暗転済みなら何もしない

	TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
}

void UMyProject1GameInstance::TransitionToAnimEventStep(int32 NextStepIndex)
{
	PendingAnimEventNextStepIndex = NextStepIndex;

	// ワープと同じ暗転合図をUIへ送る（試験実装）。WBP_LoadingScreen側が暗転→維持→明転を自走してくれる想定
	OnWarpFadeOutRequested.Broadcast();

	GetTimerManager().SetTimer(AnimEventStepTransitionTimerHandle, this,
		&UMyProject1GameInstance::HandleAnimEventStepTransitionFadeComplete, WarpFadeOutDuration, false);
}

void UMyProject1GameInstance::HandleAnimEventStepTransitionFadeComplete()
{
	const int32 NextStepIndex = PendingAnimEventNextStepIndex;
	PendingAnimEventNextStepIndex = INDEX_NONE;

	// ループ中のステップから抜ける場合は、暗転が完全に終わった今のタイミングでモンタージュを打ち切る
	// （メイン・Extra全参加者分）。Montageを明示して打ち切ることで、直後にPlayAnimEventStepが再生する
	// 次のモンタージュを巻き込まない
	if (bAnimEventStepLooping)
	{
		if (UAnimMontage* MainMontage = CurrentAnimEventMontage.Get())
		{
			if (UAnimInstance* AnimInst = ResolveAnimInstanceForTrackedMontage(MainMontage))
			{
				AnimInst->Montage_Stop(0.1f, MainMontage);
			}
		}

		for (const TPair<FName, TWeakObjectPtr<UAnimMontage>>& Pair : CurrentAnimEventExtraMontages)
		{
			UAnimMontage* ExtraMontage = Pair.Value.Get();
			if (!ExtraMontage) continue;

			if (UAnimInstance* AnimInst = ResolveAnimInstanceForTrackedMontage(ExtraMontage))
			{
				AnimInst->Montage_Stop(0.1f, ExtraMontage);
			}
		}

		bAnimEventStepLooping = false;
	}

	PlayAnimEventStep(NextStepIndex);
}

// ----------------------------------------------------
// イベント分岐システム側の統合：ClearCondition=AnimationSequence
// ----------------------------------------------------
void UMyProject1GameInstance::BeginAnimEventSequenceIfNeeded()
{
	if (!bHasActiveEvent || bAnimEventSequenceStarted || !EventDefinitionDataTable) return;

	FEventDefinition* Definition = EventDefinitionDataTable->FindRow<FEventDefinition>(ActiveEventID, TEXT("BeginAnimEventSequenceIfNeeded"));
	if (!Definition || Definition->ClearCondition != EEventClearCondition::AnimationSequence) return;

	bAnimEventSequenceStarted = true;

	if (Definition->AnimEventID.IsNone())
	{
		// アニメーションイベントが未設定なら、演出なしでそのまま成立させる（安全策）
		ResolveActiveEvent(true);
		return;
	}

	if (!OnAnimSequenceEventFinished.IsAlreadyBound(this, &UMyProject1GameInstance::HandleAnimSequenceEventFinishedForActiveEvent))
	{
		OnAnimSequenceEventFinished.AddDynamic(this, &UMyProject1GameInstance::HandleAnimSequenceEventFinishedForActiveEvent);
	}

	PlayAnimSequenceEvent(Definition->AnimEventID, ActiveEventPlayer.Get(), ActiveEventContextActor.Get(), false, ActiveEventExtraMeshOverride, bActiveEventHideContextActorDuringAnimEvent);
}

void UMyProject1GameInstance::HandleAnimSequenceEventFinishedForActiveEvent(bool bCompletedNormally)
{
	// bAnimEventSequenceStartedは「進行中イベント経由でPlayAnimSequenceEventを開始した」場合だけtrueになる。
	// 単体でのPlayAnimSequenceEvent呼び出し（会話・QuestItemPoint等、イベントを経由しない再生）の完了時は
	// bHasActiveEventがfalse、もしくはこのフラグがfalseのままなので何もしない
	if (!bHasActiveEvent || !bAnimEventSequenceStarted) return;

	ResolveActiveEvent(true);
}

