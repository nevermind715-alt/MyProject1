#include "MaineWindowWidget.h"
#include "InventoryComponent.h"
#include "MyProject1Character.h"
#include "GameFramework/PlayerController.h"

void UMaineWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwningPlayerPawn()))
	{
		CachedInventoryComp = OwnerChar->InventoryComp;
	}

	if (CachedInventoryComp)
	{
		CachedInventoryComp->OnInventoryUpdated.AddDynamic(this, &UMaineWindowWidget::HandleInventoryUpdated);
		HandleInventoryUpdated();
	}
}

void UMaineWindowWidget::NativeDestruct()
{
	if (CachedInventoryComp)
	{
		CachedInventoryComp->OnInventoryUpdated.RemoveDynamic(this, &UMaineWindowWidget::HandleInventoryUpdated);
	}

	Super::NativeDestruct();
}

void UMaineWindowWidget::HandleInventoryUpdated()
{
	if (Text_Shojikin && CachedInventoryComp)
	{
		const FText GilNumber = FText::AsNumber(CachedInventoryComp->Gil);
		Text_Shojikin->SetText(FText::Format(NSLOCTEXT("MaineWindowWidget", "GilDisplay", "所持金：￥{0}"), GilNumber));
	}
}
