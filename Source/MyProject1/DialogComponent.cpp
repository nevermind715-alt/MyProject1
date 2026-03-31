#include "DialogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject1Character.h"
#include "QuestComponent.h"

UDialogComponent::UDialogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogComponent::StartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC)
{
	if (!DialogTable) return;

	CurrentTable = DialogTable;
	CurrentNPC = InNPC;

	// データテーブルから行を取得
	FDialogData* Data = DialogTable->FindRow<FDialogData>(RowName, TEXT("DialogContext"));
	if (Data)
	{
		CurrentDialogData = *Data;
		// UIに「更新したよ！」と合図を送る
		OnDialogUpdated.Broadcast(CurrentDialogData, CurrentNPC);
	}
}

void UDialogComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!CurrentDialogData.Choices.IsValidIndex(ChoiceIndex)) return;

	const FDialogChoice& SelectedChoice = CurrentDialogData.Choices[ChoiceIndex];

	// アクションを実行
	ExecuteAction(SelectedChoice);

	// 次の会話IDがあれば移動、なければ終了（またはCloseアクションに任せる）
	if (!SelectedChoice.NextDialogID.IsNone())
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
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwner());
	if (!Player) return;

	switch (Choice.ActionType)
	{
	case EDialogActionType::AcceptQuest:
		if (Player->QuestComp)
		{
			// Payloadに入っている QuestID を使って受注
			Player->QuestComp->AcceptQuest(FName(*Choice.ActionPayload));
		}
		break;

	case EDialogActionType::ChangeFavor:
		// 文字列を数値(int)に変換して好感度を加算
		Player->MyStats.Favor += FCString::Atoi(*Choice.ActionPayload);
		Player->OnReceiveLogMessage(FString::Printf(TEXT("好感度が %s 変化した"), *Choice.ActionPayload), ELogMessageType::System);
		break;

	case EDialogActionType::Close:
		CloseDialog();
		break;

	default:
		break;
	}
}

void UDialogComponent::CloseDialog()
{
	OnDialogClosed.Broadcast();
}