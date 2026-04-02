#include "QuestComponent.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

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

	CheckInitialGatherProgress(QuestID);

	// UIに通知
	OnQuestUpdated.Broadcast(QuestID);

	// ログ表示
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
	if (OwnerChar)
	{
		FString LogMsg = FString::Printf(TEXT("クエスト「%s」を受注した。"), *Data.QuestName.ToString());
		OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
	}

	if (Data.AcceptSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Data.AcceptSound);
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
					// ★ ここで InvComp を先に取得しておく！
					UInventoryComponent* InvComp = OwnerChar->FindComponentByClass<UInventoryComponent>();
					
					if (Data.QuestType == EQuestType::Gather && InvComp)
					{
						// RemoveItem が失敗した（間違って捨ててしまって数が足りない等）場合は報告中断
						if (!InvComp->RemoveItem(Data.TargetID, Data.RequiredAmount))
						{
							if (OwnerChar) OwnerChar->OnReceiveLogMessage(TEXT("アイテムが足りません。"), ELogMessageType::System);
							return false;
						}
					}

					// --- 報酬の付与 ---
					// 1. 経験値
					if (Data.RewardExperience > 0)
					{
						OwnerChar->AddExperience(Data.RewardExperience);
					}

					// 2. ギル（インベントリを使用）
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

					if (Data.CompletionSound)
					{
						// 2Dサウンド（距離に関係なく画面全体に聞こえる音）として再生
						UGameplayStatics::PlaySound2D(GetWorld(), Data.CompletionSound);
					}
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

void UQuestComponent::UpdateGatherObjective(FName ItemID, int32 AmountAdded)
{
	bool bUpdatedAny = false;
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

	for (FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.Status != EQuestStatus::InProgress) continue;

		FQuestData Data;
		if (GetQuestData(Progress.QuestID, Data))
		{
			// 収集クエストであり、対象アイテムが一致するか
			if (Data.QuestType == EQuestType::Gather && Data.TargetID == ItemID)
			{
				Progress.CurrentAmount += AmountAdded;
				bUpdatedAny = true;

				if (Progress.CurrentAmount >= Data.RequiredAmount)
				{
					Progress.CurrentAmount = Data.RequiredAmount; // 上限で止める
					Progress.Status = EQuestStatus::ObjectiveCleared; // 報告待ち状態へ

					if (OwnerChar) {
						FString LogMsg = FString::Printf(TEXT("クエスト「%s」のアイテムが集まった！"), *Data.QuestName.ToString());
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
					}
				}
				else
				{
					// 途中経過のログ
					if (OwnerChar) {
						FString LogMsg = FString::Printf(TEXT("%sを手に入れた（%d / %d）"), *ItemID.ToString(), Progress.CurrentAmount, Data.RequiredAmount);
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);
					}
				}
			}
		}
	}
	if (bUpdatedAny) OnQuestUpdated.Broadcast(NAME_None);
}

void UQuestComponent::CheckInitialGatherProgress(FName QuestID)
{
	FQuestData Data;
	if (GetQuestData(QuestID, Data) && Data.QuestType == EQuestType::Gather)
	{
		AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
		UInventoryComponent* InvComp = OwnerChar ? OwnerChar->FindComponentByClass<UInventoryComponent>() : nullptr;

		if (InvComp)
		{
			// インベントリから現在の所持数を取得
			int32 CurrentCount = InvComp->GetItemQuantity(Data.TargetID);
			if (CurrentCount > 0)
			{
				// すでに持っていれば、その分だけカウントを進める
				UpdateGatherObjective(Data.TargetID, CurrentCount);
			}
		}
	}
}