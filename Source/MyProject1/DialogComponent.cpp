#include "DialogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "QuestComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MyProject1GameInstance.h"
#include "WarpPortal.h"
#include "MyProject1HUD.h"
#include "RpgCharacterInterface.h" 

UDialogComponent::UDialogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogComponent::StartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC)
{
	if (!DialogTable) return;

	
	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
	{
		RpgInterface->CancelTarget();
	}

	CurrentTable = DialogTable;
	CurrentNPC = InNPC;

	FDialogData* Data = DialogTable->FindRow<FDialogData>(RowName, TEXT("DialogContext"));
	if (Data)
	{
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
	}
}

void UDialogComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!CurrentDialogData.Choices.IsValidIndex(ChoiceIndex)) return;

	const FDialogChoice& SelectedChoice = CurrentDialogData.Choices[ChoiceIndex];

	ExecuteAction(SelectedChoice);

	FString NextIDStr = SelectedChoice.NextDialogID.ToString().TrimStartAndEnd();
	bool bHasNext = !SelectedChoice.NextDialogID.IsNone() && !NextIDStr.IsEmpty() && NextIDStr.ToLower() != TEXT("none");

	OnHideChoices.Broadcast();

	if (bHasNext)
	{
		StartDialog(SelectedChoice.NextDialogID, CurrentTable, CurrentNPC);
	}
	else
	{
		CloseDialog();
	}
}

void UDialogComponent::ExecuteAction(const FDialogChoice& Choice)
{
	
	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (!RpgInterface) return;

	switch (Choice.ActionType)
	{
	case EDialogActionType::AcceptQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->AcceptQuest(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::ReportQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->ReportQuest(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::AddFlag:
		RpgInterface->AddFlag(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::RemoveFlag:
		RpgInterface->RemoveFlag(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::Warp:
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
		{
			FName WarpIDToUse = FName(*Choice.ActionPayload);

			if (Choice.ActionPayload.IsEmpty() || WarpIDToUse.IsNone())
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

	case EDialogActionType::Close:
		CloseDialog();
		break;

	default:
		break;
	}

	
	if (Choice.StatToChange != ETargetStat::None && Choice.StatChangeAmount != 0.0f)
	{
		float ChangeVal = Choice.StatChangeAmount;
		FString StatName = TEXT("不明なステータス");

		// 参照(&)で受け取るため、ここで書き換えると本体のステータスに直結します
		FCharacterStats& Stats = RpgInterface->GetCharacterStats();

		switch (Choice.StatToChange)
		{
		case ETargetStat::Favor:
			Stats.Favor += ChangeVal;
			StatName = TEXT("好感度");
			break;
		case ETargetStat::Fame:
			Stats.Fame += ChangeVal;
			StatName = TEXT("名声");
			break;
		case ETargetStat::Charm:
			Stats.Charm += ChangeVal;
			StatName = TEXT("魅力");
			break;

		case ETargetStat::CustomExtraStat:
			if (!Choice.ExtraStatName.IsNone())
			{
				float* CurrentVal = Stats.ExtraStats.Find(Choice.ExtraStatName);

				if (CurrentVal)
				{
					*CurrentVal += ChangeVal;
					StatName = Choice.ExtraStatName.ToString();
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("【Dialog Error】 ExtraStat '%s' が登録されていません！"), *Choice.ExtraStatName.ToString());
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
	else
	{
		StartDialog(CurrentDialogData.NextDialogID, CurrentTable, CurrentNPC);
	}
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