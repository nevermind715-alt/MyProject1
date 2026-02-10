// WBP_ShopSlot.cpp
#include "WBP_ShopSlot.h"
#include "MyProject1Character.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.h"

void UWBP_ShopSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (HoverSound)
    {
        UGameplayStatics::PlaySound2D(this, HoverSound);
    }

    if (AMyProject1Character* PC = Cast<AMyProject1Character>(GetOwningPlayerPawn()))
    {
        if (UInventoryComponent* Inv = PC->FindComponentByClass<UInventoryComponent>())
        {
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
            Inv->ReportItemHover(NAME_None); // ó£ÇÍÇΩÇÁãÛÅiNoneÅjÇïÒçê
        }
    }
}