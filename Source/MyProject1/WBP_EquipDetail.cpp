#include "WBP_EquipDetail.h"
#include "InventoryComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

void UWBP_EquipDetail::UpdateEquipDetail(FName ItemID, bool bVisible)
{
	if (!bVisible)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	APawn* PlayerPawn = GetOwningPlayerPawn();
	UInventoryComponent* Inv = PlayerPawn ? PlayerPawn->FindComponentByClass<UInventoryComponent>() : nullptr;

	FItemData Data;
	if (ItemDetails && Inv && Inv->GetItemDataBP(ItemID, Data))
	{
		ItemDetails->SetText(Data.Description);
	}
}