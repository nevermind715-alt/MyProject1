#include "QuestComponent.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"

#pragma execution_character_set("utf-8")

UQuestComponent::UQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 毎フレーム処理は不要なので軽くする
}

bool UQuestComponent::GetQuestData(FName QuestID, FQuestData& OutData)
{
	if (QuestID.IsNone() || !QuestDataTable) return false;

	FQuestData* Data = QuestDataTable->FindRow<FQuestData>(QuestID, TEXT("QuestContext"));
	if (Data)
	{
		OutData = *Data;
		return true;
	}
	return false;
}

EQuestStatus UQuestComponent::GetQuestStatus(FName QuestID)
{
	// 1. すでにクリア済みかチェック
	if (CompletedQuests.Contains(QuestID))
	{
		return EQuestStatus::Completed;
	}

	// 2. 進行中リストにあるかチェック
	for (const FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.QuestID == QuestID)
		{
			return Progress.Status;
		}
	}

	// 3. どちらにも無ければ未受注
	return EQuestStatus::NotStarted;
}

bool UQuestComponent::AcceptQuest(FName QuestID)
{
	// すでに受注している、またはクリア済みなら受けられない
	if (GetQuestStatus(QuestID) != EQuestStatus::NotStarted)
	{
		return false;
	}

	// データテーブルにそのクエストが存在するか確認
	FQuestData Data;
	if (!GetQuestData(QuestID, Data))
	{
		return false;
	}

	// 進行中リストに追加
	FQuestProgress NewQuest(QuestID);
	ActiveQuests.Add(NewQuest);

	// UIに通知
	OnQuestUpdated.Broadcast(QuestID);

	// ログ表示
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
	if (OwnerChar)
	{
		FString LogMsg = FString::Printf(TEXT("クエスト「%s」を受注した。"), *Data.QuestName.ToString());
		OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
	}

	return true;
}

void UQuestComponent::UpdateKillObjective(FName EnemyID)
{
	bool bUpdatedAny = false;

	// 進行中の全クエストをチェック
	for (FQuestProgress& Progress : ActiveQuests)
	{
		// 進行中でない場合はスキップ
		if (Progress.Status != EQuestStatus::InProgress) continue;

		FQuestData Data;
		if (GetQuestData(Progress.QuestID, Data))
		{
			// 討伐クエストであり、かつ倒した敵のIDが目標と一致するか？
			if (Data.QuestType == EQuestType::Kill && Data.TargetID == EnemyID)
			{
				// カウントを増やす
				Progress.CurrentAmount++;
				bUpdatedAny = true;

				AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

				// 目標数に達したか？
				if (Progress.CurrentAmount >= Data.RequiredAmount)
				{
					Progress.CurrentAmount = Data.RequiredAmount; // 上限で止める
					Progress.Status = EQuestStatus::ObjectiveCleared; // 報告待ち状態へ

					if (OwnerChar)
					{
						FString LogMsg = FString::Printf(TEXT("クエスト「%s」の目的を達成した！"), *Data.QuestName.ToString());
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
					}
				}
				else
				{
					// 途中経過のログ
					if (OwnerChar)
					{
						FString LogMsg = FString::Printf(TEXT("%sを倒した（%d / %d）"), *EnemyID.ToString(), Progress.CurrentAmount, Data.RequiredAmount);
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);
					}
				}
			}
		}
	}

	// 1つでも進行したクエストがあればUIに通知
	if (bUpdatedAny)
	{
		OnQuestUpdated.Broadcast(NAME_None); // 全体更新
	}
}

bool UQuestComponent::ReportQuest(FName QuestID)
{
	// 進行中リストから該当クエストを探す
	for (int32 i = 0; i < ActiveQuests.Num(); ++i)
	{
		if (ActiveQuests[i].QuestID == QuestID)
		{
			// 「条件達成」状態でないと報告できない
			if (ActiveQuests[i].Status != EQuestStatus::ObjectiveCleared)
			{
				return false;
			}

			FQuestData Data;
			if (GetQuestData(QuestID, Data))
			{
				AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
				if (OwnerChar)
				{
					// --- 報酬の付与 ---
					// 1. 経験値
					if (Data.RewardExperience > 0)
					{
						OwnerChar->AddExperience(Data.RewardExperience);
					}

					// 2. ギル（インベントリを使用）
					UInventoryComponent* InvComp = OwnerChar->FindComponentByClass<UInventoryComponent>();
					if (InvComp)
					{
						if (Data.RewardGil > 0)
						{
							InvComp->AddGil(Data.RewardGil);
							FString GilMsg = FString::Printf(TEXT("報酬として %d￥ 手に入れた。"), Data.RewardGil);
							OwnerChar->OnReceiveLogMessage(GilMsg, ELogMessageType::System);
						}

						// 3. アイテム
						if (!Data.RewardItemID.IsNone() && Data.RewardItemAmount > 0)
						{
							InvComp->AddItem(Data.RewardItemID, Data.RewardItemAmount);
							// ※AddItemの中でログを出す設定があればここは不要かもしれません
						}
					}

					// 完了ログ
					FString LogMsg = FString::Printf(TEXT("クエスト「%s」をコンプリートした！"), *Data.QuestName.ToString());
					OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
				}

				// リストの移動（進行中から消して、完了履歴に追加）
				ActiveQuests.RemoveAt(i);
				CompletedQuests.Add(QuestID);

				OnQuestUpdated.Broadcast(QuestID);
				return true;
			}
		}
	}

	return false;
}