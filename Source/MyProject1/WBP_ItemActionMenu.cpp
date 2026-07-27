#include "WBP_ItemActionMenu.h"
#include "InventoryComponent.h"
#include "GameFramework/Pawn.h"

void UWBP_ItemActionMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (APawn* PlayerPawn = GetOwningPlayerPawn())
	{
		if (UInventoryComponent* Inv = PlayerPawn->FindComponentByClass<UInventoryComponent>())
		{
			Inv->OnItemActionMenuForceClose.AddDynamic(this, &UWBP_ItemActionMenu::HandleForceClose);
		}
	}
}

void UWBP_ItemActionMenu::HandleForceClose()
{
	RemoveFromParent();
}
