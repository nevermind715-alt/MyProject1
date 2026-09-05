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

	ExecuteActionCore(SelectedChoice.ActionType, SelectedChoice.ActionPayload, SelectedChoice.GrantFlag, SelectedChoice.bFadeOnGrantFlag, SelectedChoice.RemoveFlag, SelectedChoice.bFadeOnRemoveFlag, SelectedChoice.StatToChange, SelectedChoice.StatTargetActor, SelectedChoice.ExtraStatName, SelectedChoice.StatChangeAmount, SelectedChoice.ItemID, SelectedChoice.ItemAmount, SelectedChoice.bAdvanceDailySequence);

	FString NextIDStr = SelectedChoice.NextDialogID.ToString().TrimStartAndEnd();
	bool bHasNext = !SelectedChoice.NextDialogID.IsNone() && !NextIDStr.IsEmpty() && NextIDStr.ToLower() != TEXT("none");

	OnHideChoices.Broadcast();

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

void UDialogComponent::ExecuteActionCore(EDialogActionType ActionType, const FString& ActionPayload, FName GrantFlag, bool bFadeOnGrantFlag, FName FlagToRemove, bool bFadeOnRemoveFlag, ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount, FName ItemID, int32 ItemAmount, bool bAdvanceDailySequence)
{

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (!RpgInterface) return;

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

	switch (ActionType)
	{
	case EDialogActionType::AcceptQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->AcceptQuest(FName(*ActionPayload));
		break;

	case EDialogActionType::ReportQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->ReportQuest(FName(*ActionPayload));
		break;

	case EDialogActionType::CancelQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->CancelQuest(FName(*ActionPayload));
		break;

	case EDialogActionType::TalkProgress:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
		{
			// 話しかけた相手（NPC）の識別はActorのTags（詳細パネルで設定）を使う。親クラスを問わず使えるようにするため
			QuestComp->UpdateTalkObjective(FName(*ActionPayload), CurrentNPC);
		}
		break;

	case EDialogActionType::AddFlag:
		RpgInterface->AddFlag(FName(*ActionPayload));
		break;

	case EDialogActionType::RemoveFlag:
		RpgInterface->RemoveFlag(FName(*ActionPayload));
		break;

	case EDialogActionType::Warp:
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
		{
			FName WarpIDToUse = FName(*ActionPayload);

			if (ActionPayload.IsEmpty() || WarpIDToUse.IsNone())
			{
				if (AWarpPortal* Portal = Cast<AWarpPortal>(CurrentNPC))
				{
					WarpIDToUse = Portal->TargetWarpID;
				}
			}

			if (!WarpIDToUse.IsNone())
			{
				// ワープ関数はACharacterを要求するため、OwnerをACharacterにキャストして渡す
				if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
				{
					GameInst->RequestWarp(WarpIDToUse, OwnerChar);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ワープIDが設定されていません！"));
			}
		}
		break;

	case EDialogActionType::RequestRankUp:
		// ActionPayloadに設定先の等級（EAdventurerRankの行名。例："Rank4"）を入れて使う
		RpgInterface->SetAdventurerRank(FName(*ActionPayload));
		break;

	case EDialogActionType::AddItem:
		// ItemID/ItemAmountで指定したアイテムをプレイヤーのインベントリに追加する（渡す）。
		// カバンが満杯で入り切らない場合はAddItem側がfalseを返すが、ここでは通知は出さない
		if (!ItemID.IsNone() && ItemAmount > 0)
		{
			if (UInventoryComponent* Inv = GetOwner()->FindComponentByClass<UInventoryComponent>())
				Inv->AddItem(ItemID, ItemAmount);
		}
		break;

	case EDialogActionType::RemoveItem:
		// ItemID/ItemAmountで指定したアイテムをプレイヤーのインベントリから削除する（奪う）。
		// UInventoryComponent::RemoveItem自体はログを出さない（ポーション消費など内部利用で無言にするため）ので、
		// 会話でのアイテム受け渡し時だけここでログを出す（AddItem側はAddItem内でログが出るのと対になる）
		if (!ItemID.IsNone() && ItemAmount > 0)
		{
			if (UInventoryComponent* Inv = GetOwner()->FindComponentByClass<UInventoryComponent>())
			{
				if (Inv->RemoveItem(ItemID, ItemAmount))
				{
					FItemData ItemInfo;
					const FString ItemName = Inv->GetItemDataBP(ItemID, ItemInfo) ? ItemInfo.Name : ItemID.ToString();
					const FString LogMsg = (ItemAmount == 1)
						? FString::Printf(TEXT("%sを渡した。"), *ItemName)
						: FString::Printf(TEXT("%sを%d個渡した。"), *ItemName, ItemAmount);
					RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
				}
			}
		}
		break;

	case EDialogActionType::Close:
		CloseDialog();
		break;

	default:
		break;
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

	if (StatToChange != ETargetStat::None && StatChangeAmount != 0.0f)
	{
		float ChangeVal = StatChangeAmount;
		FString StatName = TEXT("不明なステータス");

		// StatTargetActor=NPCなら、話しかけている相手自身のMyStats（個体ごとのFavor/Hostility等）を書き換える。
		// キャストに失敗した場合（NPCが未設定、IRpgCharacterInterface非対応など）はプレイヤー側にフォールバックする
		IRpgCharacterInterface* StatOwnerInterface = RpgInterface;
		if (StatTargetActor == EStatTargetActor::NPC)
		{
			if (IRpgCharacterInterface* NPCInterface = Cast<IRpgCharacterInterface>(CurrentNPC))
			{
				StatOwnerInterface = NPCInterface;
			}
		}

		// 参照(&)で受け取るため、ここで書き換えると本体のステータスに直結します
		FCharacterStats& Stats = StatOwnerInterface->GetCharacterStats();

		switch (StatToChange)
		{
		case ETargetStat::Favor:
			Stats.Favor += ChangeVal;
			StatName = TEXT("好感度");
			break;
		case ETargetStat::Hostility:
			Stats.Hostility += ChangeVal;
			StatName = TEXT("敵対度");
			break;
		case ETargetStat::Fame:
			Stats.Fame += ChangeVal;
			StatName = TEXT("名声");
			break;
		case ETargetStat::Charm:
			Stats.Charm += ChangeVal;
			StatName = TEXT("魅力");
			break;

		case ETargetStat::Alcohol:
			Stats.Alcohol += ChangeVal;
			StatName = TEXT("酒量");
			break;

		case ETargetStat::CustomExtraStat:
			if (!ExtraStatName.IsNone())
			{
				float* CurrentVal = Stats.ExtraStats.Find(ExtraStatName);

				if (CurrentVal)
				{
					*CurrentVal += ChangeVal;
					StatName = ExtraStatName.ToString();
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("【Dialog Error】 ExtraStat '%s' が登録されていません！"), *ExtraStatName.ToString());
					ChangeVal = 0.0f;
				}
			}
			else
			{
				ChangeVal = 0.0f;
			}
			break;
		default:
			break;
		}

		if (ChangeVal != 0.0f)
		{
			FString Sign = (ChangeVal > 0) ? TEXT("上がった") : TEXT("下がった");
			FString LogMsg = FString::Printf(TEXT("%sが %.0f %s。"), *StatName, FMath::Abs(ChangeVal), *Sign);

			// ログもUI通知もインターフェース経由
			RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
			RpgInterface->NotifyStatsChanged();
		}
	}
}

void UDialogComponent::CloseDialog()
{

	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
	{
		RpgInterface->CancelTarget();
		RpgInterface->SetInputLocked(false);
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

	OnDialogClosed.Broadcast();
}

void UDialogComponent::AdvanceDialog()
{
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

	if (RpgInterface->GetCharacterStats().Mental < Choice.RequiredMental)
	{
		return false;
	}

	if (!Choice.RequiredFlag.IsNone())
	{
		if (!RpgInterface->HasFlag(Choice.RequiredFlag))
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

	OnDialogUpdated.Broadcast(DisplayData, CurrentNPC);

	if (CurrentLineIndex == CurrentDialogLines.Num() - 1)
	{
		// 選択肢がない会話は、このセリフに直接設定されたアクションをここで実行する
		// （会話終了設定の場合、この下でAdvanceDialogを経由せず直接CloseDialogするため、ここで実行しないと機会を失う）
		if (CurrentDialogData.Choices.Num() == 0)
		{
			ExecuteActionCore(CurrentDialogData.ActionType, CurrentDialogData.ActionPayload, CurrentDialogData.GrantFlag, CurrentDialogData.bFadeOnGrantFlag, CurrentDialogData.RemoveFlag, CurrentDialogData.bFadeOnRemoveFlag, CurrentDialogData.StatToChange, CurrentDialogData.StatTargetActor, CurrentDialogData.ExtraStatName, CurrentDialogData.StatChangeAmount, CurrentDialogData.ItemID, CurrentDialogData.ItemAmount, CurrentDialogData.bAdvanceDailySequence);
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