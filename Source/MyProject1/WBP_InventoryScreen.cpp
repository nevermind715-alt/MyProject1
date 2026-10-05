#include "WBP_InventoryScreen.h"
#include "WBP_InventorySlot.h"
#include "InventoryComponent.h"
#include "MyProject1Character.h"
#include "MyProject1HUD.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "GameFramework/PlayerController.h"

UInventoryComponent* UWBP_InventoryScreen::GetInventory() const
{
	APawn* PlayerPawn = GetOwningPlayerPawn();
	return PlayerPawn ? PlayerPawn->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UWBP_InventoryScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// ボタンは1回だけバインドする（ウィジェットごとに初期化は1度のみ）
	if (Btn_Wepon)    Btn_Wepon->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandleWeaponClicked);
	if (Btn_Potion)   Btn_Potion->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandlePotionClicked);
	if (Btn_Armor)    Btn_Armor->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandleArmorClicked);
	if (Btn_Food)     Btn_Food->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandleFoodClicked);
	if (Btn_material) Btn_material->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandleMaterialClicked);
	if (Btn_etc)      Btn_etc->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandleEtcClicked);
	if (Btn_KeyItem)  Btn_KeyItem->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandleKeyItemClicked);
	if (Btn_Close)    Btn_Close->OnClicked.AddDynamic(this, &UWBP_InventoryScreen::HandleCloseClicked);
}

void UWBP_InventoryScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (UInventoryComponent* Inv = GetInventory())
	{
		Inv->OnInventoryUpdated.AddUniqueDynamic(this, &UWBP_InventoryScreen::HandleInventoryUpdated);
	}

	RefreshList();
}

void UWBP_InventoryScreen::NativeDestruct()
{
	if (UInventoryComponent* Inv = GetInventory())
	{
		Inv->OnInventoryUpdated.RemoveDynamic(this, &UWBP_InventoryScreen::HandleInventoryUpdated);

		// 画面を閉じた時にアクションメニューが残らないようにする
		if (Inv->bIsItemActionMenuOpen)
		{
			Inv->SetItemActionMenuState(false);
		}
		Inv->ReportItemHover(NAME_None);
	}

	Super::NativeDestruct();
}

void UWBP_InventoryScreen::SelectTab(EItemType NewTab)
{
	ActiveTab = NewTab;
	RefreshList();
}

void UWBP_InventoryScreen::RefreshList()
{
	if (!ItemScrollBox) return;

	ItemScrollBox->ClearChildren();

	UInventoryComponent* Inv = GetInventory();
	if (!InventorySlotClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] InventorySlotClass が未設定。WBP_InventoryScreen のクラスデフォルトで WBP_InventorySlot を指定してください。"));
		return;
	}
	if (InventorySlotClass && InventorySlotClass->IsNative())
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] InventorySlotClass にC++クラス自体が指定されている。Blueprintアセット(WBP_InventorySlot)を選び直してください。"));
		return;
	}
	if (!Inv) return;

	const AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	APlayerController* PC = GetOwningPlayer();

	for (const FInventorySlot& Entry : Inv->GetInventoryByType(ActiveTab))
	{
		UWBP_InventorySlot* SlotWidget = CreateWidget<UWBP_InventorySlot>(PC, InventorySlotClass);
		if (!SlotWidget) continue;

		SlotWidget->SetupSlot(Entry.ItemID, Entry.Quantity, Player && Player->IsItemEquipped(Entry.ItemID));
		ItemScrollBox->AddChild(SlotWidget);
	}
}

void UWBP_InventoryScreen::HandleInventoryUpdated()
{
	RefreshList();
}

void UWBP_InventoryScreen::CloseInventoryScreen()
{
	APlayerController* PC = GetOwningPlayer();
	if (AMyProject1HUD* HUD = PC ? Cast<AMyProject1HUD>(PC->GetHUD()) : nullptr)
	{
		HUD->ToggleInventoryMenu();
	}
}

void UWBP_InventoryScreen::HandleWeaponClicked()   { SelectTab(EItemType::Weapon); }
void UWBP_InventoryScreen::HandlePotionClicked()   { SelectTab(EItemType::Potion); }
void UWBP_InventoryScreen::HandleArmorClicked()    { SelectTab(EItemType::Armor); }
void UWBP_InventoryScreen::HandleFoodClicked()     { SelectTab(EItemType::Food); }
void UWBP_InventoryScreen::HandleMaterialClicked() { SelectTab(EItemType::Material); }
void UWBP_InventoryScreen::HandleEtcClicked()      { SelectTab(EItemType::Consumable); }
void UWBP_InventoryScreen::HandleKeyItemClicked()  { SelectTab(EItemType::KeyItem); }
void UWBP_InventoryScreen::HandleCloseClicked()    { CloseInventoryScreen(); }