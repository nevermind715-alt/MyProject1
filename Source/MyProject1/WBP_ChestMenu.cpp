#include "WBP_ChestMenu.h"
#include "WBP_ChestItemSlot.h"
#include "ChestComponent.h"
#include "MyProject1Character.h"
#include "MyProject1HUD.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "GameFramework/PlayerController.h"

void UWBP_ChestMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_Close)
	{
		Btn_Close->OnClicked.AddDynamic(this, &UWBP_ChestMenu::HandleCloseClicked);
	}
}

void UWBP_ChestMenu::NativeConstruct()
{
	Super::NativeConstruct();

	// HUDはこのウィジェットを使い回すため、開くたびに(NativeConstruct)作り直す
	RefreshList();
}

void UWBP_ChestMenu::RefreshList()
{
	if (!ChestListScroll) return;

	ChestListScroll->ClearChildren();

	if (!ChestSlotClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ChestMenu] ChestSlotClass が未設定。WBP_ChestMenu のクラスデフォルトで WBP_ChestItemSlot を指定してください。"));
		return;
	}

	if (ChestSlotClass && ChestSlotClass->IsNative())
	{
		UE_LOG(LogTemp, Error, TEXT("[ChestMenu] ChestSlotClass にC++クラス自体が指定されている。Blueprintアセット(WBP_ChestItemSlot)を選び直してください。"));
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	AMyProject1HUD* HUD = PC ? Cast<AMyProject1HUD>(PC->GetHUD()) : nullptr;
	UChestComponent* Chest = HUD ? HUD->CurrentChestComp : nullptr;
	if (!Chest) return;

	for (const FInventorySlot& Entry : Chest->GetChestContents())
	{
		UWBP_ChestItemSlot* SlotWidget = CreateWidget<UWBP_ChestItemSlot>(PC, ChestSlotClass);
		if (!SlotWidget) continue;

		// NativeConstructで表示するため、AddChildの前に値を渡す
		SlotWidget->ItemID = Entry.ItemID;
		SlotWidget->Quantity = Entry.Quantity;
		SlotWidget->TargetChest = Chest;
		ChestListScroll->AddChild(SlotWidget);
	}
}

void UWBP_ChestMenu::HandleCloseClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (AMyProject1HUD* HUD = PC ? Cast<AMyProject1HUD>(PC->GetHUD()) : nullptr)
	{
		HUD->ToggleChestMenu();
	}

	// 元のBPと同じく、閉じたらターゲットも解除する
	if (AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn()))
	{
		Player->CancelTarget();
	}
}