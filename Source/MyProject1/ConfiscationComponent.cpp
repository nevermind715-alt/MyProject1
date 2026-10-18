#include "ConfiscationComponent.h"
#include "Engine/DataTable.h"
#include "InventoryComponent.h"
#include "MyProject1Character.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

UConfiscationComponent::UConfiscationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UConfiscationComponent::ConfiscateByProfile(FName ProfileID)
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwner());
	UInventoryComponent* Inv = Player ? Player->InventoryComp : nullptr;
	if (!Player || !Inv || !Player->EquipmentDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Confiscation] 没収できない: オーナーがプレイヤーでない、またはInventoryComp/EquipmentDataTableが未設定 (Profile=%s)"), *ProfileID.ToString());
		return false;
	}
	if (!ProfileDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Confiscation] ProfileDataTableが未設定 (Profile=%s)。BP_Player側のConfiscationCompに設定すること"), *ProfileID.ToString());
		return false;
	}

	const FConfiscationProfile* Profile = ProfileDataTable->FindRow<FConfiscationProfile>(ProfileID, TEXT("ConfiscateByProfile"));
	if (!Profile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Confiscation] ProfileDataTableに行 '%s' が無い"), *ProfileID.ToString());
		return false;
	}

	const FName StashID = Profile->StashID.IsNone() ? ProfileID : Profile->StashID;
	FConfiscatedStash& Stash = Stashes.FindOrAdd(StashID);
	Stash.bReEquipOnReturn = Profile->bReEquipOnReturn;

	const TSet<FName> OutfitIDs(Profile->OutfitRowNames);
	bool bChanged = false;

	// 1. 装備を外す。RemoveItemは装備中の分を減らせないため、カバンから取り上げるより先に行う
	TArray<EEquipmentSlot> EquippedSlots;
	Player->CurrentEquippedItems.GetKeys(EquippedSlots);
	for (const EEquipmentSlot Slot : EquippedSlots)
	{
		if (Profile->SlotsToKeep.Contains(Slot)) continue;

		const FName ItemID = Player->GetEquippedItemID(Slot);
		if (ItemID.IsNone() || OutfitIDs.Contains(ItemID)) continue;
		if (!Profile->bConfiscateLockedEquipment && Player->IsSlotLocked(Slot)) continue;

		// すでに同じ保管先に没収品がある（再び捕まった）場合、装備の記録は最初の没収時のものを残す
		if (!Stash.Equipment.Contains(Slot))
		{
			Stash.Equipment.Add(Slot, ItemID);
		}
		Player->UnequipItem(Slot);
		bChanged = true;
	}

	// 2. カバンの中身を取り上げる
	TMap<FName, int32> Totals;
	for (const FInventorySlot& InvSlot : Inv->InventoryContent)
	{
		Totals.FindOrAdd(InvSlot.ItemID) += InvSlot.Quantity;
	}

	for (const TPair<FName, int32>& Pair : Totals)
	{
		const FName ItemID = Pair.Key;
		if (OutfitIDs.Contains(ItemID) || Profile->KeepItemIDs.Contains(ItemID)) continue;

		FItemData ItemData;
		if (!Inv->GetItemDataBP(ItemID, ItemData)) continue;
		if (Profile->bKeepKeyItems && ItemData.ItemType == EItemType::KeyItem) continue;
		if (Profile->bKeepExItems && ItemData.bIsEx) continue;

		// 没収対象外のスロットに装備したままの分は、RemoveItemが拒否するので最初から除く
		const int32 Amount = Pair.Value - Player->GetEquippedCount(ItemID);
		if (Amount <= 0 || !Inv->RemoveItem(ItemID, Amount)) continue;

		if (FInventorySlot* Existing = Stash.Items.FindByPredicate([ItemID](const FInventorySlot& S) { return S.ItemID == ItemID; }))
		{
			Existing->Quantity += Amount;
		}
		else
		{
			FInventorySlot NewSlot;
			NewSlot.ItemID = ItemID;
			NewSlot.Quantity = Amount;
			Stash.Items.Add(NewSlot);
		}
		bChanged = true;
	}

	// 3. 所持金を取り上げる
	const int32 GilToTake = FMath::FloorToInt(static_cast<double>(Inv->Gil) * FMath::Clamp(Profile->GilConfiscateRatio, 0.0f, 1.0f));
	if (GilToTake > 0)
	{
		Stash.Gil += Inv->RemoveGil(GilToTake);
		bChanged = true;
	}

	// 4. 服を着せる
	for (const FName& RowName : Profile->OutfitRowNames)
	{
		if (RowName.IsNone()) continue;

		const FEquipmentData* EquipData = Player->EquipmentDataTable->FindRow<FEquipmentData>(RowName, TEXT("ConfiscateByProfile Outfit"));
		if (!EquipData)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Confiscation] 服 '%s' がEquipmentDataTableに無い (Profile=%s)"), *RowName.ToString(), *ProfileID.ToString());
			continue;
		}

		// 再び捕まって、すでに同じ服を着ているなら着せ直さない
		if (Player->GetEquippedItemID(EquipData->TargetSlot) == RowName)
		{
			Stash.OutfitRowNames.AddUnique(RowName);
			continue;
		}

		// ロック装備を残す設定のとき、そのスロットへ服を着せると拘束が外れてしまうので着せない
		if (Player->IsSlotLocked(EquipData->TargetSlot)) continue;

		// 装備中のアイテムはカバンにも入っている仕様のため、服もカバンに入れてから着せる
		if (!Inv->EnsureAtLeast(RowName, 1))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Confiscation] 服 '%s' をカバンに入れられなかった（DT_Itemsに同名の行が無い、またはカバンが満杯）"), *RowName.ToString());
		}
		Player->EquipItem(RowName, *EquipData);
		Stash.OutfitRowNames.AddUnique(RowName);
		bChanged = true;
	}

	if (!bChanged)
	{
		// 何も没収せず、服も着せなかった（新規に作った空の保管先は残さない）
		if (Stash.Items.Num() == 0 && Stash.Equipment.Num() == 0 && Stash.Gil == 0 && Stash.OutfitRowNames.Num() == 0)
		{
			Stashes.Remove(StashID);
		}
		return false;
	}

	Player->OnReceiveLogMessage(TEXT("持ち物を没収された。"), ELogMessageType::System);
	return true;
}

bool UConfiscationComponent::ReturnStash(FName StashID)
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwner());
	UInventoryComponent* Inv = Player ? Player->InventoryComp : nullptr;
	if (!Player || !Inv || !Player->EquipmentDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Confiscation] 返却できない: オーナーがプレイヤーでない、またはInventoryComp/EquipmentDataTableが未設定 (Stash=%s)"), *StashID.ToString());
		return false;
	}

	const FConfiscatedStash* Found = Stashes.Find(StashID);
	if (!Found) return false;

	// 装備の着脱の途中でStashesが変わっても安全なよう、コピーで作業して最後に書き戻す
	FConfiscatedStash Stash = *Found;

	// 1. 着せていた服を脱がせる（ロック装備でも外す。カバンからも消す）
	if (Stash.bReEquipOnReturn)
	{
		for (const FName& RowName : Stash.OutfitRowNames)
		{
			const FEquipmentData* EquipData = Player->EquipmentDataTable->FindRow<FEquipmentData>(RowName, TEXT("ReturnStash Outfit"));
			if (!EquipData) continue;

			if (Player->GetEquippedItemID(EquipData->TargetSlot) == RowName)
			{
				Player->UnequipItem(EquipData->TargetSlot);
			}
			if (Player->GetEquippedCount(RowName) == 0 && Inv->GetItemQuantity(RowName) > 0)
			{
				Inv->RemoveItem(RowName, 1);
			}
		}
		Stash.OutfitRowNames.Reset();
	}

	// 2. 持ち物をカバンへ戻す（入手ログ・クエスト通知・レア制限なし）。入り切らなかった分は保管に残す
	TArray<FInventorySlot> NotReturned;
	for (const FInventorySlot& StashSlot : Stash.Items)
	{
		const int32 Before = Inv->GetItemQuantity(StashSlot.ItemID);
		Inv->EnsureAtLeast(StashSlot.ItemID, Before + StashSlot.Quantity);
		const int32 Added = Inv->GetItemQuantity(StashSlot.ItemID) - Before;
		if (Added < StashSlot.Quantity)
		{
			FInventorySlot Rest;
			Rest.ItemID = StashSlot.ItemID;
			Rest.Quantity = StashSlot.Quantity - Added;
			NotReturned.Add(Rest);
		}
	}
	Stash.Items = NotReturned;

	// 3. 所持金を戻す
	if (Stash.Gil > 0)
	{
		Inv->AddGil(Stash.Gil);
		Stash.Gil = 0;
	}

	// 4. 没収時の装備を着せ直す。まだカバンに戻っていない装備は、次の返却まで記録を残す
	if (Stash.bReEquipOnReturn)
	{
		for (auto It = Stash.Equipment.CreateIterator(); It; ++It)
		{
			const FName ItemID = It.Value();
			if (Inv->GetItemQuantity(ItemID) <= 0) continue;

			// 返却中に別の装備でロックされたスロットには着せない（カバンには戻っているので、後から自分で装備できる）
			const FEquipmentData* EquipData = Player->EquipmentDataTable->FindRow<FEquipmentData>(ItemID, TEXT("ReturnStash Equip"));
			if (EquipData && !Player->IsSlotLocked(EquipData->TargetSlot))
			{
				Player->EquipItem(ItemID, *EquipData);
			}
			It.RemoveCurrent();
		}
	}
	else
	{
		Stash.Equipment.Reset();
	}

	if (Stash.Items.Num() == 0 && Stash.Equipment.Num() == 0)
	{
		Stashes.Remove(StashID);
		Player->OnReceiveLogMessage(TEXT("没収された持ち物を取り戻した。"), ELogMessageType::System);
	}
	else
	{
		Stashes.Add(StashID, Stash);
		Player->OnReceiveLogMessage(TEXT("持ち物を取り戻したが、カバンに入り切らない分が残った。"), ELogMessageType::System);
	}
	return true;
}

void UConfiscationComponent::ReturnAllStashes()
{
	TArray<FName> StashIDs;
	Stashes.GetKeys(StashIDs);
	for (const FName& StashID : StashIDs)
	{
		ReturnStash(StashID);
	}
}
