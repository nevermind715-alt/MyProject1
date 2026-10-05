#include "MaineWindowWidget.h"
#include "InventoryComponent.h"
#include "MyProject1Character.h"
#include "MyProject1GameInstance.h"
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

	CachedGameInstance = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (CachedGameInstance)
	{
		CachedGameInstance->OnCycleDisplayChanged.AddDynamic(this, &UMaineWindowWidget::HandleCycleDisplayChanged);
		HandleCycleDisplayChanged();
	}
}

void UMaineWindowWidget::NativeDestruct()
{
	if (CachedInventoryComp)
	{
		CachedInventoryComp->OnInventoryUpdated.RemoveDynamic(this, &UMaineWindowWidget::HandleInventoryUpdated);
	}

	if (CachedGameInstance)
	{
		CachedGameInstance->OnCycleDisplayChanged.RemoveDynamic(this, &UMaineWindowWidget::HandleCycleDisplayChanged);
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

void UMaineWindowWidget::HandleCycleDisplayChanged()
{
	if (Text_ki && CachedGameInstance)
	{
		Text_ki->SetText(CachedGameInstance->GetCurrentCycleDisplayName());
	}
}
