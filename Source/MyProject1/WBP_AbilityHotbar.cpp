#include "WBP_AbilityHotbar.h"
#include "WBP_AbilitySlot.h"
#include "AbilityComponent.h"

void UWBP_AbilityHotbar::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Box_Slots || !SlotWidgetClass) return;

	// NativeConstructはAddToViewportのたびに呼ばれ得るため、積み重ならないよう毎回作り直す
	Box_Slots->ClearChildren();

	for (int32 SlotIndex = 0; SlotIndex < UAbilityComponent::NumHotbarSlots; ++SlotIndex)
	{
		UWBP_AbilitySlot* SlotWidget = CreateWidget<UWBP_AbilitySlot>(this, SlotWidgetClass);
		if (!SlotWidget) continue;

		SlotWidget->Setup(SlotIndex);
		Box_Slots->AddChild(SlotWidget);
	}
}
