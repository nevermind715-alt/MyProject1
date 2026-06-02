#include "WBP_ShopDetail.h"
#include "InventoryComponent.h"
#include "GameFramework/Pawn.h" // ’Ç‰Á

void UWBP_ShopDetail::HandleHoverChanged(FName NewItemID)
{
	APawn* PlayerPawn = GetOwningPlayerPawn();
	UInventoryComponent* Inv = PlayerPawn ? PlayerPawn->FindComponentByClass<UInventoryComponent>() : nullptr;

	if (!Inv)
	{
		OnUpdateDetailDisplay(FItemData(), false);
		return;
	}

	FItemData Data;
	if (!NewItemID.IsNone() && Inv && Inv->GetItemDataBP(NewItemID, Data))
	{
		OnUpdateDetailDisplay(Data, true);
	}
	else
	{
		OnUpdateDetailDisplay(FItemData(), false);
	}
}

void UWBP_ShopDetail::NativeConstruct()
{
	Super::NativeConstruct();

	if (APawn* PlayerPawn = GetOwningPlayerPawn())
	{
		if (UInventoryComponent* Inv = PlayerPawn->FindComponentByClass<UInventoryComponent>())
		{
			Inv->OnItemHoverChanged.AddDynamic(this, &UWBP_ShopDetail::HandleHoverChanged);
		}
	}
}