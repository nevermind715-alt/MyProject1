#include "WBP_InventorySlot.h"
#include "WBP_ItemActionMenu.h"
#include "InventoryComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UWBP_InventorySlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UWBP_InventorySlot::HandleSlotClicked);
		SlotButton->OnClicked.AddDynamic(this, &UWBP_InventorySlot::HandleSlotClicked);
	}
}

void UWBP_InventorySlot::SetupSlot(FName InItemID, int32 InQuantity, bool bInEquipped)
{
	ItemID = InItemID;

	APawn* PlayerPawn = GetOwningPlayerPawn();
	UInventoryComponent* Inv = PlayerPawn ? PlayerPawn->FindComponentByClass<UInventoryComponent>() : nullptr;

	FItemData Data;
	const bool bHasData = Inv && Inv->GetItemDataBP(InItemID, Data);

	FString DisplayName = bHasData ? Data.Name : InItemID.ToString();
	if (bInEquipped && !EquippedMark)
	{
		DisplayName += TEXT("【装備中】");
	}

	if (NameText)
	{
		NameText->SetText(FText::FromString(DisplayName));
	}
	if (QuantityText)
	{
		QuantityText->SetText(FText::AsNumber(InQuantity));
	}
	if (IconImage)
	{
		if (bHasData && Data.Icon)
		{
			IconImage->SetBrushFromTexture(Data.Icon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (EquippedMark)
	{
		EquippedMark->SetVisibility(bInEquipped ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWBP_InventorySlot::HandleSlotClicked()
{
	APlayerController* PC = GetOwningPlayer();
	APawn* PlayerPawn = GetOwningPlayerPawn();
	UInventoryComponent* Inv = PlayerPawn ? PlayerPawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!ActionMenuClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventorySlot] ActionMenuClass が未設定。WBP_InventorySlot のクラスデフォルトで WBP_ItemActionMenu を指定してください。"));
		return;
	}
	if (ActionMenuClass && ActionMenuClass->IsNative())
	{
		UE_LOG(LogTemp, Error, TEXT("[InventorySlot] ActionMenuClass にC++クラス自体が指定されている。Blueprintアセット(WBP_ItemActionMenu)を選び直してください。"));
		return;
	}
	if (!PC || !Inv || ItemID.IsNone()) return;

	// 既に開いているアクションメニューがあれば閉じる（別のスロットを押し直した場合）
	TArray<UUserWidget*> OldMenus;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, OldMenus, UWBP_ItemActionMenu::StaticClass(), true);
	for (UUserWidget* Old : OldMenus)
	{
		Old->RemoveFromParent();
	}

	Inv->SetItemActionMenuState(true);

	UWBP_ItemActionMenu* Menu = CreateWidget<UWBP_ItemActionMenu>(PC, ActionMenuClass);
	if (!Menu)
	{
		Inv->SetItemActionMenuState(false);
		return;
	}
	Menu->TargetItemID = ItemID;

	// 位置は指定しない。WBP_ItemActionMenuのDesignerで置いた場所に固定で出る。
	Menu->AddToViewport(25);
}