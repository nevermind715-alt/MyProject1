#include "WBP_ItemActionMenu.h"
#include "InventoryComponent.h"
#include "Components/Button.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UInventoryComponent* UWBP_ItemActionMenu::GetInventory() const
{
	APawn* PlayerPawn = GetOwningPlayerPawn();
	return PlayerPawn ? PlayerPawn->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UWBP_ItemActionMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (UInventoryComponent* Inv = GetInventory())
	{
		Inv->OnItemActionMenuForceClose.AddUniqueDynamic(this, &UWBP_ItemActionMenu::HandleForceClose);
	}

	// NativeConstructは再AddToViewportで再度呼ばれることがあるため、二重バインドを避けて一度外してから付ける
	if (Btn_Use)
	{
		Btn_Use->OnClicked.RemoveDynamic(this, &UWBP_ItemActionMenu::HandleUseClicked);
		Btn_Use->OnClicked.AddDynamic(this, &UWBP_ItemActionMenu::HandleUseClicked);
	}
	if (Btn_Drop)
	{
		Btn_Drop->OnClicked.RemoveDynamic(this, &UWBP_ItemActionMenu::HandleDropClicked);
		Btn_Drop->OnClicked.AddDynamic(this, &UWBP_ItemActionMenu::HandleDropClicked);
	}
	if (Btn_Cancel)
	{
		Btn_Cancel->OnClicked.RemoveDynamic(this, &UWBP_ItemActionMenu::HandleCancelClicked);
		Btn_Cancel->OnClicked.AddDynamic(this, &UWBP_ItemActionMenu::HandleCancelClicked);
	}
}

void UWBP_ItemActionMenu::HandleForceClose()
{
	RemoveFromParent();
}

void UWBP_ItemActionMenu::HandleUseClicked()
{
	if (UInventoryComponent* Inv = GetInventory())
	{
		Inv->UseItem(TargetItemID);
	}
	CloseMenu(UseSound);
}

void UWBP_ItemActionMenu::HandleDropClicked()
{
	// 装備中・EXアイテムの場合はDiscardItem側がログを出して拒否する
	if (UInventoryComponent* Inv = GetInventory())
	{
		Inv->DiscardItem(TargetItemID, 1);
	}
	CloseMenu(DropSound);
}

void UWBP_ItemActionMenu::HandleCancelClicked()
{
	CloseMenu(CancelSound);
}

void UWBP_ItemActionMenu::CloseMenu(USoundBase* Sound)
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}

	if (UInventoryComponent* Inv = GetInventory())
	{
		// falseにすると ForceClose が配信され、自分自身（と他に残っているメニュー）が閉じる
		Inv->SetItemActionMenuState(false);
		// 詳細パネルの選択を解除
		Inv->ReportItemHover(NAME_None);
	}

	RemoveFromParent();
}