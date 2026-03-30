// InventoryComponent.cpp

#include "InventoryComponent.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "MyProject1Character.h"
#include "QuestComponent.h"

#pragma execution_character_set("utf-8")

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Tickは使わないのでOFFにして軽量化
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

FItemData* UInventoryComponent::GetItemData(FName ItemID)
{
	// ★追加: IDが None の場合は探さずにすぐ帰る（これでログの警告が消えます）
	if (ItemID.IsNone() || !ItemDataTable) return nullptr;

	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("InventoryContext"));
}

bool UInventoryComponent::AddItem(FName ItemID, int32 Amount)
{
	if (Amount <= 0) return false;

	FItemData* ItemInfo = GetItemData(ItemID);
	if (!ItemInfo)
	{
		// データテーブルに存在しない謎のアイテム
		// ここでログを出すだけで false を返せば、BP側で「無視」扱いになります
		UE_LOG(LogTemp, Warning, TEXT("Item ID [%s] not found in DataTable. Ignoring."), *ItemID.ToString());
		return false;
	}

	// 残りの追加したい数
	int32 RemainingAmount = Amount;

	// 1. 既に持っているスロットを探して、スタック（重ねる）できるか試す
	for (FInventorySlot& Slot : InventoryContent)
	{
		if (Slot.ItemID == ItemID)
		{
			// まだスタックできる余裕があるか？
			int32 SpaceInSlot = ItemInfo->MaxStack - Slot.Quantity;

			if (SpaceInSlot > 0)
			{
				// 重ねられるだけ重ねる
				int32 AddCount = FMath::Min(RemainingAmount, SpaceInSlot);

				Slot.Quantity += AddCount;
				RemainingAmount -= AddCount;

				if (RemainingAmount <= 0)
				{
					// ★ 追加：既存枠に綺麗に重なった場合もクエストに通知する
					AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
					if (OwnerChar && OwnerChar->QuestComp)
					{
						OwnerChar->QuestComp->UpdateGatherObjective(ItemID, AddCount);
					}

					// 全部入ったので終了
					OnInventoryUpdated.Broadcast();
					return true;
				}
			}
		}
	}

	// 2. まだ残っている場合、新しいスロット（空き枠）を使う
	while (RemainingAmount > 0)
	{
		// カバンがいっぱいかチェック
		if (InventoryContent.Num() >= MaxSlots)
		{
			// UI更新（入った分だけ反映）
			OnInventoryUpdated.Broadcast();
			// 「カバンがいっぱいでこれ以上持てない」状態
			return false;
		}

		// 新しいスロットを作る
		FInventorySlot NewSlot;
		NewSlot.ItemID = ItemID;

		// 1スロットに入る最大数まで入れる
		int32 AddCount = FMath::Min(RemainingAmount, ItemInfo->MaxStack);
		NewSlot.Quantity = AddCount;

		InventoryContent.Add(NewSlot);
		RemainingAmount -= AddCount;
	}

	// ★ ここを追加：カバンに入ったアイテムをクエスト側に通知する
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
	if (OwnerChar && OwnerChar->QuestComp)
	{
		// Amount から、カバンがいっぱいで入りきらなかった分(RemainingAmount)を引いた「実質的な取得数」を渡す
		int32 ActuallyAdded = Amount - RemainingAmount;
		if (ActuallyAdded > 0)
		{
			OwnerChar->QuestComp->UpdateGatherObjective(ItemID, ActuallyAdded);
		}
	}

	// ここまで来たら完全に収納成功
	OnInventoryUpdated.Broadcast();
	return true;
}

bool UInventoryComponent::RemoveItem(FName ItemID, int32 Amount)
{
	if (Amount <= 0) return false;

	// まず持っている総数をチェック
	if (GetItemQuantity(ItemID) < Amount)
	{
		return false; // 足りない
	}

	int32 RemainingToRemove = Amount;

	// 後ろのスロットから消していく（インデックス操作の都合上安全）
	for (int32 i = InventoryContent.Num() - 1; i >= 0; --i)
	{
		if (InventoryContent[i].ItemID == ItemID)
		{
			if (InventoryContent[i].Quantity > RemainingToRemove)
			{
				// このスロットから一部だけ減らして終了
				InventoryContent[i].Quantity -= RemainingToRemove;
				RemainingToRemove = 0;
				break;
			}
			else
			{
				// このスロットを使い切る（削除する）
				RemainingToRemove -= InventoryContent[i].Quantity;
				InventoryContent.RemoveAt(i);
			}

			if (RemainingToRemove <= 0) break;
		}
	}

	OnInventoryUpdated.Broadcast();
	return true;
}

int32 UInventoryComponent::GetItemQuantity(FName ItemID)
{
	int32 Total = 0;
	for (const FInventorySlot& Slot : InventoryContent)
	{
		if (Slot.ItemID == ItemID)
		{
			Total += Slot.Quantity;
		}
	}
	return Total;
}

void UInventoryComponent::AddGil(int32 Amount)
{
	Gil += Amount;
	// 上限チェック（9億9999万ギルなど）を入れるならここ
	OnInventoryUpdated.Broadcast();
}

bool UInventoryComponent::TrySpendGil(int32 Amount)
{
	if (Gil >= Amount)
	{
		Gil -= Amount;
		OnInventoryUpdated.Broadcast();
		return true;
	}
	return false;
}

bool UInventoryComponent::UseItem(FName ItemID)
{
	if (GetItemQuantity(ItemID) <= 0) return false;
	FItemData* ItemInfo = GetItemData(ItemID);
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
	if (!ItemInfo || !OwnerChar) return false;

	if (!ItemInfo->UseSound.IsNull())
	{
		// ソフトポインタを同期ロード
		USoundBase* SoundToPlay = ItemInfo->UseSound.LoadSynchronous();
		if (SoundToPlay)
		{
			// キャラクターの位置で再生（FF11のように周囲にも聞こえる）
			UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, OwnerChar->GetActorLocation());
		}
	}

	TArray<FItemEffect> TimedEffects;
	float TotalHealed = 0.0f; // ★追加：回復量の合計
	bool bAnyEffectApplied = false;

	for (const FItemEffect& Effect : ItemInfo->Effects)
	{
		switch (Effect.TargetStat)
		{
		case ETargetStat::HP:
			if (Effect.EffectDuration <= 0.0f)
			{
				// ★ UpdateHealth の戻り値（実際の回復量）を足していく
				TotalHealed += OwnerChar->UpdateHealth(Effect.EffectAmount);
				bAnyEffectApplied = true;
			}
			else
			{
				TimedEffects.Add(Effect);
				bAnyEffectApplied = true;
			}
			break;

		case ETargetStat::STR:
		case ETargetStat::DEX:
		case ETargetStat::VIT:
		case ETargetStat::AGI:
		case ETargetStat::Stamina:
		case ETargetStat::Accuracy:
		case ETargetStat::Evasion:
		case ETargetStat::AttackPower:
			if (Effect.EffectDuration > 0.0f)
			{
				TimedEffects.Add(Effect);
			}
			bAnyEffectApplied = true;
			break;
		default:
			break;
		}
	}

	// バフをまとめて1回だけ実行
	if (TimedEffects.Num() > 0)
	{
		OwnerChar->ApplyItemBuff(ItemInfo->Name, TimedEffects, TimedEffects[0].EffectDuration);
	}

	if (bAnyEffectApplied)
	{
		// 1. 使用ログ（FF11風：〇〇を使用した。）
		FString LogMsg = FString::Printf(TEXT("%sを使用した。"), *ItemInfo->Name);
		OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);

		// 2. ★復活：回復ログ（FF11風：HPが 50 回復した。）
		if (TotalHealed > 0.0f)
		{
			FString HealMsg = FString::Printf(TEXT("HPが %.0f 回復した。"), TotalHealed);
			OwnerChar->OnReceiveLogMessage(HealMsg, ELogMessageType::System);
		}

		if (ItemInfo->bConsumeOnUse) { RemoveItem(ItemID, 1); }
	}
	return bAnyEffectApplied;
}

bool UInventoryComponent::ProcessPurchase(FName ItemID, int32 Amount)
{
	FItemData* ItemInfo = GetItemData(ItemID); // アイテム情報を取得
	if (!ItemInfo || Amount <= 0) return false;

	int32 TotalPrice = ItemInfo->Price * Amount; // 合計金額
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

	// 1. お金が足りるかチェック
	if (Gil < TotalPrice)
	{
		if (OwnerChar)
		{
			OwnerChar->OnReceiveLogMessage(TEXT("お金が足りません。"), ELogMessageType::System);
		}
		return false;
	}

	// 2. アイテムを追加（AddItemの中でカバンの空きをチェックしている）
	if (AddItem(ItemID, Amount))
	{
		// 3. 成功したらお金を支払う
		TrySpendGil(TotalPrice);

		// 4. システムログを送信（FF11風のメッセージ）
		if (OwnerChar)
		{
			FString LogMsg = FString::Printf(TEXT("%sを%d個買った。%d￥支払った。"), *ItemInfo->Name, Amount, TotalPrice);
			OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
		}
		return true;
	}
	else
	{
		// カバンがいっぱいで追加できなかった場合
		if (OwnerChar)
		{
			OwnerChar->OnReceiveLogMessage(TEXT("カバンがいっぱいで買えません。"), ELogMessageType::System);
		}
		return false;
	}
}

bool UInventoryComponent::ProcessSale(FName ItemID, int32 Amount)
{
	FItemData* ItemInfo = GetItemData(ItemID);
	if (!ItemInfo) return false;

	// 1. 「だいじなもの」チェック（売却不可なら即終了）
	if (ItemInfo->ItemType == EItemType::KeyItem)
	{
		AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
		if (OwnerChar)
		{
			OwnerChar->OnReceiveLogMessage(TEXT("それは売却できません。"), ELogMessageType::System);
		}
		return false;
	}

	// 2. 所持数の確認
	if (GetItemQuantity(ItemID) < Amount) return false;

	// 3. 売却価格の計算（データテーブルの SellPrice を使用）
	int32 TotalSellPrice = ItemInfo->SellPrice * Amount;

	// 4. アイテムの削除とギルの加算
	if (RemoveItem(ItemID, Amount))
	{
		AddGil(TotalSellPrice);

		// システムログの送信
		AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
		if (OwnerChar)
		{
			FString LogMsg = FString::Printf(TEXT("%sを%d個売却した。%d￥手に入れた。"), *ItemInfo->Name, Amount, TotalSellPrice);
			OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
		}
		return true;
	}

	return false;
}

bool UInventoryComponent::GetItemDataBP(FName ItemID, FItemData& OutData)
{
	FItemData* Data = GetItemData(ItemID);
	if (Data)
	{
		OutData = *Data; // 見つかったら中身をコピーして渡す
		return true;
	}
	return false; // 見つからなければ false
}


void UInventoryComponent::SetItemActionMenuState(bool bIsOpen)
{
	bIsItemActionMenuOpen = bIsOpen;
}
