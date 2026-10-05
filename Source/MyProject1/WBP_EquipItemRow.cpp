#include "WBP_EquipItemRow.h"
#include "WBP_EquipItemList.h"
#include "WBP_EquipDetail.h"
#include "InventoryComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

void UWBP_EquipItemRow::SetupRow(FName InItemID, UWBP_EquipItemList* InParentList, UWBP_EquipDetail* InDetail, bool bInEquipped)
{
	ItemID = InItemID;
	ParentList = InParentList;
	DetailWidget = InDetail;
	bEquipped = bInEquipped;
	RefreshName();
}

void UWBP_EquipItemRow::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_SelectItem)
	{
		Btn_SelectItem->OnClicked.RemoveDynamic(this, &UWBP_EquipItemRow::HandleSelectClicked);
		Btn_SelectItem->OnClicked.AddDynamic(this, &UWBP_EquipItemRow::HandleSelectClicked);
	}
	RefreshName();
}

void UWBP_EquipItemRow::RefreshName()
{
	if (!Text_ItemName || ItemID.IsNone()) return;

	FString DisplayName = ItemID.ToString();
	APawn* PlayerPawn = GetOwningPlayerPawn();
	if (UInventoryComponent* Inv = PlayerPawn ? PlayerPawn->FindComponentByClass<UInventoryComponent>() : nullptr)
	{
		FItemData Data;
		if (Inv->GetItemDataBP(ItemID, Data))
		{
			DisplayName = Data.Name;
		}
	}
	if (bEquipped)
	{
		DisplayName += TEXT("【装備中】");
	}
	Text_ItemName->SetText(FText::FromString(DisplayName));
}

void UWBP_EquipItemRow::HandleSelectClicked()
{
	if (ParentList)
	{
		ParentList->OnRowSelected(ItemID);
	}
}

void UWBP_EquipItemRow::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (DetailWidget)
	{
		DetailWidget->UpdateEquipDetail(ItemID, true);
	}
}

void UWBP_EquipItemRow::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (DetailWidget)
	{
		DetailWidget->UpdateEquipDetail(NAME_None, false);
	}
}