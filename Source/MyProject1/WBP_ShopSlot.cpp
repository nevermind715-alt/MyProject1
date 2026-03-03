// WBP_ShopSlot.cpp
#include "WBP_ShopSlot.h"
#include "MyProject1Character.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.h"

void UWBP_ShopSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (AMyProject1Character* PC = Cast<AMyProject1Character>(GetOwningPlayerPawn()))
    {
        if (UInventoryComponent* Inv = PC->FindComponentByClass<UInventoryComponent>())
        {
            // ★追加：サブメニューが開いている（True）なら、ここで処理を中断する
            if (Inv->bIsItemActionMenuOpen)
            {
                return;
            }

            // サブメニューが開いていない時だけ、音を鳴らして報告する
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

    if (AMyProject1Character* PC = Cast<AMyProject1Character>(GetOwningPlayerPawn()))
    {
        if (UInventoryComponent* Inv = PC->FindComponentByClass<UInventoryComponent>())
        {
            if (!Inv->bIsItemActionMenuOpen)
            {
                Inv->ReportItemHover(NAME_None);
            }
        }
    }
}