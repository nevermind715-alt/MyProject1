// InventoryComponent.cpp

#include "InventoryComponent.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "MyProject1Character.h"
#include "QuestComponent.h"
#include "RpgCharacterInterface.h"

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
	// IDが None の場合は探さずにすぐ帰る（これでログの警告が消えます）
	if (ItemID.IsNone() || !ItemDataTable) return nullptr;

	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("InventoryContext"));
}

bool UInventoryComponent::AddItem(FName ItemID, int32 Amount)
{
	if (Amount <= 0) return false;

	FItemData* ItemInfo = GetItemData(ItemID);
	if (!ItemInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item ID [%s] not found in DataTable. Ignoring."), *ItemID.ToString());
		return false;
	}

	int32 RemainingAmount = Amount;

	// 1. 既に持っているスロットを探して、スタック（重ねる）できるか試す
	for (FInventorySlot& Slot : InventoryContent)
	{
		if (Slot.ItemID == ItemID)
		{
			int32 SpaceInSlot = ItemInfo->MaxStack - Slot.Quantity;
			if (SpaceInSlot > 0)
			{
				int32 AddCount = FMath::Min(RemainingAmount, SpaceInSlot);
				Slot.Quantity += AddCount;
				RemainingAmount -= AddCount;

				// 全部入ったらループを抜ける
				if (RemainingAmount <= 0) break;
			}
		}
	}

	// 2. まだ残っている場合、新しいスロット（空き枠）を使う
	while (RemainingAmount > 0)
	{
		if (InventoryContent.Num() >= MaxSlots)
		{
			break; // カバンがいっぱいでこれ以上入らない
		}

		FInventorySlot NewSlot;
		NewSlot.ItemID = ItemID;

		int32 AddCount = FMath::Min(RemainingAmount, ItemInfo->MaxStack);
		NewSlot.Quantity = AddCount;

		InventoryContent.Add(NewSlot);
		RemainingAmount -= AddCount;
	}

	// 実際にカバンに入った数を計算
	int32 ActuallyAdded = Amount - RemainingAmount;

	// 1個でもカバンに入った場合の処理
	if (ActuallyAdded > 0)
	{
		// クエストへの通知などは、まだ依存関係があるため一旦既存のキャラクターへのキャストを残します
		AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
		if (OwnerChar && OwnerChar->QuestComp)
		{
			OwnerChar->QuestComp->UpdateGatherObjective(ItemID, ActuallyAdded);
		}

		// ★ログ出力部分をインターフェース（窓口）経由に変更！
		// これにより、将来プレイヤー以外のNPCがアイテムを拾った時でもエラーにならずログが流せます
		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
		{
			FString LogMsg;
			if (ActuallyAdded == 1)
			{
				LogMsg = FString::Printf(TEXT("%sを手に入れた。"), *ItemInfo->Name);
			}
			else
			{
				LogMsg = FString::Printf(TEXT("%sを%d個手に入れた。"), *ItemInfo->Name, ActuallyAdded);
			}
			RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
		}

		// UI更新
		OnInventoryUpdated.Broadcast();
	}

	return RemainingAmount <= 0;
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

TArray<FInventorySlot> UInventoryComponent::GetInventoryByType(EItemType TargetType)
{
	TArray<FInventorySlot> FilteredSlots;

	for (const FInventorySlot& Slot : InventoryContent)
	{
		FItemData* ItemInfo = GetItemData(Slot.ItemID);
		if (ItemInfo && ItemInfo->ItemType == TargetType)
		{
			FilteredSlots.Add(Slot);
		}
	}

	return FilteredSlots;
}

TArray<FInventorySlot> UInventoryComponent::GetInventoryByTypes(const TArray<EItemType>& TargetTypes)
{
	TArray<FInventorySlot> FilteredSlots;

	for (const FInventorySlot& Slot : InventoryContent)
	{
		FItemData* ItemInfo = GetItemData(Slot.ItemID);
		if (ItemInfo && TargetTypes.Contains(ItemInfo->ItemType))
		{
			FilteredSlots.Add(Slot);
		}
	}

	return FilteredSlots;
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

	// ---消費アイテム以外（武器・防具・素材・だいじなもの等）なら使えないように弾く ---
	if (ItemInfo->ItemType != EItemType::Consumable &&
		ItemInfo->ItemType != EItemType::Potion &&
		ItemInfo->ItemType != EItemType::Food)
	{
		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
		{
			FString CannotUseMsg = FString::Printf(TEXT("%s は使えない。"), *ItemInfo->Name);
			RpgInterface->OnReceiveLogMessage(CannotUseMsg, ELogMessageType::System);
		}
		return false;
	}

	// ---bCanUseがfalseなら効果を発動させず使用不可ログのみ出す ---
	if (!ItemInfo->bCanUse)
	{
		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
		{
			FString CannotUseMsg = FString::Printf(TEXT("%s は使用できない！"), *ItemInfo->Name);
			RpgInterface->OnReceiveLogMessage(CannotUseMsg, ELogMessageType::System);
		}
		return false;
	}

	if (!ItemInfo->UseSound.IsNull())
	{
		USoundBase* SoundToPlay = ItemInfo->UseSound.LoadSynchronous();
		if (SoundToPlay)
		{
			UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, OwnerChar->GetActorLocation());
		}
	}

	TArray<FItemEffect> TimedEffects;
	float TotalHealed = 0.0f;
	float TotalStaminaHealed = 0.0f; // ← 【追加】スタミナ回復量を記録する変数
	bool bAnyEffectApplied = false;

	for (const FItemEffect& Effect : ItemInfo->Effects)
	{
		switch (Effect.TargetStat)
		{
		case ETargetStat::HP:
			if (Effect.EffectDuration <= 0.0f)
			{
				TotalHealed += OwnerChar->UpdateHealth(Effect.EffectAmount);
				bAnyEffectApplied = true;
			}
			else
			{
				TimedEffects.Add(Effect);
				bAnyEffectApplied = true;
			}
			break;

			// ▼▼ Stamina を他のステータスから独立させる ▼▼
		case ETargetStat::Stamina:
			if (Effect.EffectDuration <= 0.0f)
			{
				// スタミナの即時回復処理
				float OldStamina = OwnerChar->MyStats.Stamina;
				OwnerChar->MyStats.Stamina = FMath::Clamp(OwnerChar->MyStats.Stamina + Effect.EffectAmount, 0.0f, OwnerChar->MyStats.MaxStamina);
				TotalStaminaHealed += (OwnerChar->MyStats.Stamina - OldStamina);

				// UIのスタミナバーをリアルタイムで更新する合図を送る
				if (OwnerChar->OnStaminaChangedDelegate.IsBound())
				{
					OwnerChar->OnStaminaChangedDelegate.Broadcast(OwnerChar->MyStats.Stamina, OwnerChar->MyStats.MaxStamina);
				}
				bAnyEffectApplied = true;
			}
			else
			{
				TimedEffects.Add(Effect);
				bAnyEffectApplied = true;
			}
			break;

		case ETargetStat::Fame:
			OwnerChar->MyStats.Fame += Effect.EffectAmount;
			bAnyEffectApplied = true;
			break;

		case ETargetStat::Favor:
			OwnerChar->MyStats.Favor += Effect.EffectAmount;
			bAnyEffectApplied = true;
			break;

		case ETargetStat::Charm:
			OwnerChar->MyStats.Charm += Effect.EffectAmount;
			bAnyEffectApplied = true;
			break;

		case ETargetStat::CustomExtraStat:
			if (!Effect.ExtraStatName.IsNone())
			{
				float CurrentValue = OwnerChar->MyStats.ExtraStats.FindRef(Effect.ExtraStatName);
				OwnerChar->MyStats.ExtraStats.Add(Effect.ExtraStatName, CurrentValue + Effect.EffectAmount);
				bAnyEffectApplied = true;
			}
			break;

		case ETargetStat::STR:
		case ETargetStat::DEX:
		case ETargetStat::VIT:
		case ETargetStat::AGI:
		case ETargetStat::Accuracy:
		case ETargetStat::Evasion:
		case ETargetStat::AttackPower:
		case ETargetStat::DefensePower:
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

	if (bAnyEffectApplied)
	{
		
		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
		{
			FString LogMsg = FString::Printf(TEXT("%sを使用した。"), *ItemInfo->Name);
			RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::System);

			if (TotalHealed > 0.0f)
			{
				FString HealMsg = FString::Printf(TEXT("HPが %.0f 回復した。"), TotalHealed);
				RpgInterface->OnReceiveLogMessage(HealMsg, ELogMessageType::System);
			}

			if (TotalStaminaHealed > 0.0f)
			{
				FString StaminaMsg = FString::Printf(TEXT("スタミナが %.0f 回復した。"), TotalStaminaHealed);
				RpgInterface->OnReceiveLogMessage(StaminaMsg, ELogMessageType::System);
			}
		}

		if (ItemInfo->bConsumeOnUse) { RemoveItem(ItemID, 1); }
	}

	if (TimedEffects.Num() > 0)
	{
		OwnerChar->ApplyItemBuff(ItemInfo->Name, ItemInfo->Icon, TimedEffects, TimedEffects[0].EffectDuration);
	}

	return bAnyEffectApplied;
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

	// 閉じる時は、実際にUIを消す処理が誰から呼ばれても確実に反映されるよう合図を出す
	if (!bIsOpen)
	{
		OnItemActionMenuForceClose.Broadcast();
	}
}
