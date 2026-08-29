// WBP_ShopSlot.cpp
#include "WBP_ShopSlot.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.h"
#include "GameFramework/Pawn.h"

void UWBP_ShopSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (APawn* PlayerPawn = GetOwningPlayerPawn())
    {
        if (UInventoryComponent* Inv = PlayerPawn->FindComponentByClass<UInventoryComponent>())
        {
            if (Inv->bIsItemActionMenuOpen)
            {
                return;
            }

            if (HoverSound)
            {
                UGameplayStatics::PlaySound2D(this, HoverSound);
            }

            Inv->ReportItemHover(ItemID);
        }
    }
}

void UWBP_ShopSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (APawn* PlayerPawn = GetOwningPlayerPawn())
    {
        if (UInventoryComponent* Inv = PlayerPawn->FindComponentByClass<UInventoryComponent>())
        {
            // アクションメニュー表示中は、そちらへカーソルが移動しただけで
            // 詳細パネルを消してしまわないようにホバー解除を無視する
            if (Inv->bIsItemActionMenuOpen)
            {
                return;
            }

            // 離れたのでNone(未選択)を送る
            Inv->ReportItemHover(NAME_None);
        }
    }
}
