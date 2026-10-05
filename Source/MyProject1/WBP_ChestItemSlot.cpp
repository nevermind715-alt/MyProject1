#include "WBP_ChestItemSlot.h"
#include "ChestComponent.h"
#include "InventoryComponent.h"
#include "MyProject1Character.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UWBP_ChestItemSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Take)
	{
		Btn_Take->OnClicked.RemoveDynamic(this, &UWBP_ChestItemSlot::HandleTakeClicked);
		Btn_Take->OnClicked.AddDynamic(this, &UWBP_ChestItemSlot::HandleTakeClicked);
	}

	// アイテム名はインベントリ側のItemDataTableから引く（ItemIDはスポーン時に渡される）
	if (Text_ItemName)
	{
		FString DisplayName = ItemID.ToString();
		if (const AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn()))
		{
			FItemData Data;
			if (Player->InventoryComp && Player->InventoryComp->GetItemDataBP(ItemID, Data))
			{
				DisplayName = Data.Name;
			}
		}
		Text_ItemName->SetText(FText::FromString(DisplayName));
	}

	UpdateQuantityText();
}

void UWBP_ChestItemSlot::UpdateQuantityText()
{
	if (Text_Quantity)
	{
		Text_Quantity->SetText(FText::AsNumber(Quantity));
	}
}

void UWBP_ChestItemSlot::HandleTakeClicked()
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	if (!TargetChest || !Player) return;

	if (!TargetChest->TakeItem(ItemID, 1, Player)) return;

	// 無限チェストは減らない
	if (TargetChest->bIsInfinite) return;

	--Quantity;
	if (Quantity <= 0)
	{
		RemoveFromParent();
	}
	else
	{
		UpdateQuantityText();
	}
}