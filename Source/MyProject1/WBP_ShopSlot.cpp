// WBP_ShopSlot.cpp
#include "WBP_ShopSlot.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComponent.h"
#include "GameFramework/Pawn.h" // 追加：APawnの基本機能を使うため

void UWBP_ShopSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    // ★特定のキャラクター名を名指しせず、ただの「操作中のPawn」として扱う
    if (APawn* PlayerPawn = GetOwningPlayerPawn())
    {
        // 相手が誰であれ、InventoryComponentを持っているか探す
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
            // 離れた時はNone(空っぽ)を送る
            Inv->ReportItemHover(NAME_None);
        }
    }
}