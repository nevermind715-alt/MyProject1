#include "DialogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject1Character.h"
#include "QuestComponent.h"
#include "GameFramework/Character.h"

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

		// ボイスが設定されていれば鳴らす
		if (CurrentDialogData.DialogVoice)
		{
			// 距離に関係なくハッキリ聞こえるように2Dサウンドで再生
			UGameplayStatics::PlaySound2D(GetWorld(), CurrentDialogData.DialogVoice);
		}

		// エモートが設定されていれば、NPCに再生させる
		if (CurrentDialogData.DialogEmote && CurrentNPC)
		{
			// 渡されたNPCアクターを「Character」として扱い、モンタージュを再生する
			ACharacter* NPCCharacter = Cast<ACharacter>(CurrentNPC);
			if (NPCCharacter)
			{
				NPCCharacter->PlayAnimMontage(CurrentDialogData.DialogEmote);
			}
		}

		// 選択肢が0個、かつ「次の会話」も設定されていない場合のみ終了 ---
		if (CurrentDialogData.Choices.Num() == 0 && CurrentDialogData.NextDialogID.IsNone())
		{
			CloseDialog();
		}

		// 選択肢が0個の場合は、返事を待たずに即座に会話を終了する（ロックを解除する）
		if (CurrentDialogData.Choices.Num() == 0)
		{
			CloseDialog();
		}
	}
}

void UDialogComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!CurrentDialogData.Choices.IsValidIndex(ChoiceIndex)) return;

	const FDialogChoice& SelectedChoice = CurrentDialogData.Choices[ChoiceIndex];

	// アクションを実行
	ExecuteAction(SelectedChoice);

	// 次の会話IDがあれば移動、なければ終了（またはCloseアクションに任せる）
	FString NextIDStr = SelectedChoice.NextDialogID.ToString().TrimStartAndEnd();
	bool bHasNext = !SelectedChoice.NextDialogID.IsNone() && !NextIDStr.IsEmpty() && NextIDStr.ToLower() != TEXT("none");

	if (bHasNext)
	{
		StartDialog(SelectedChoice.NextDialogID, CurrentTable, CurrentNPC);
	}
	else
	{
		// 次のセリフがないなら、アクションの種類に関わらず必ず閉じる
		CloseDialog();
	}
}

void UDialogComponent::ExecuteAction(const FDialogChoice& Choice)
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwner());
	if (!Player) return;

	// 1. メインのアクションを実行（受注、報告、UIを閉じるなど）
	switch (Choice.ActionType)
	{
	case EDialogActionType::AcceptQuest:
		if (Player->QuestComp) Player->QuestComp->AcceptQuest(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::ReportQuest:
		if (Player->QuestComp) Player->QuestComp->ReportQuest(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::Close:
		CloseDialog();
		break;

		// ※ ChangeStat は下で独立して処理するので、ここには書かなくてOKです
	default:
		break;
	}

	// 2. ★独立処理：ステータス変化の設定があれば、上のアクションとは「別枠」で必ず実行する！
	if (Choice.StatToChange != ETargetStat::None && Choice.StatChangeAmount != 0.0f)
	{
		float ChangeVal = Choice.StatChangeAmount;
		FString StatName = TEXT("不明なステータス");

		switch (Choice.StatToChange)
		{
		case ETargetStat::Favor:
			Player->MyStats.Favor += ChangeVal;
			StatName = TEXT("好感度");
			break;
		case ETargetStat::Fame:
			Player->MyStats.Fame += ChangeVal;
			StatName = TEXT("名声");
			break;
		case ETargetStat::Charm:
			Player->MyStats.Charm += ChangeVal;
			StatName = TEXT("魅力");
			break;

			// 他のステータスも必要ならここに追加可能
		case ETargetStat::CustomExtraStat:
			if (!Choice.ExtraStatName.IsNone())
			{
				// ★ Find は「探すだけ」。見つからなければ nullptr（空っぽ）を返す
				float* CurrentVal = Player->MyStats.ExtraStats.Find(Choice.ExtraStatName);

				if (CurrentVal)
				{
					// 見つかった場合だけ数値を足す
					*CurrentVal += ChangeVal;
					StatName = Choice.ExtraStatName.ToString();
				}
				else
				{
					// ★ 見つからなかった場合（タイポなどのミス）
					// 開発者向けにアウトプットログに黄色い警告文字を出す！
					UE_LOG(LogTemp, Warning, TEXT("【Dialog Error】 ExtraStat '%s' が MyStats に登録されていません！"), *Choice.ExtraStatName.ToString());

					// 画面上のログやステータス更新をスキップさせるために0にする
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

		// ログに結果を表示
		if (ChangeVal != 0.0f)
		{
			FString Sign = (ChangeVal > 0) ? TEXT("上がった") : TEXT("下がった");
			FString LogMsg = FString::Printf(TEXT("%sが %.0f %s。"), *StatName, FMath::Abs(ChangeVal), *Sign);
			Player->OnReceiveLogMessage(LogMsg, ELogMessageType::System);

			// UI（ステータス画面など）を更新する合図
			Player->NotifyStatsChanged();
		}
	}
}

void UDialogComponent::CloseDialog()
{
	OnDialogClosed.Broadcast();
}

void UDialogComponent::AdvanceDialog()
{
	// 次の会話IDが設定されていればそこへ進み、設定されていなければ会話を終了する
	if (!CurrentDialogData.NextDialogID.IsNone())
	{
		StartDialog(CurrentDialogData.NextDialogID, CurrentTable, CurrentNPC);
	}
	else
	{
		CloseDialog();
	}
}