#include "WBP_AbilityHotbar.h"
#include "WBP_AbilitySlot.h"
#include "AbilityComponent.h"
#include "Components/UniformGridPanel.h"

void UWBP_AbilityHotbar::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Box_Slots || !SlotWidgetClass) return;

	// NativeConstructはAddToViewportのたびに呼ばれ得るため、積み重ならないよう毎回作り直す
	Box_Slots->ClearChildren();
	Slots.Empty();

	for (int32 SlotIndex = 0; SlotIndex < UAbilityComponent::NumHotbarSlots; ++SlotIndex)
	{
		UWBP_AbilitySlot* SlotWidget = CreateWidget<UWBP_AbilitySlot>(this, SlotWidgetClass);
		if (!SlotWidget) continue;

		SlotWidget->Setup(SlotIndex);
		SlotWidget->OnSlotClicked.AddDynamic(this, &UWBP_AbilityHotbar::HandleSlotClicked);

		// UniformGridPanelなら6個ごとに折り返す（キー7〜0が、キー1〜4の真下に並ぶ）。それ以外の箱は従来どおり順に追加する
		if (UUniformGridPanel* Grid = Cast<UUniformGridPanel>(Box_Slots))
		{
			Grid->AddChildToUniformGrid(SlotWidget, SlotIndex / SlotsPerRow, SlotIndex % SlotsPerRow);
		}
		else
		{
			Box_Slots->AddChild(SlotWidget);
		}
		Slots.Add(SlotWidget);
	}
}

void UWBP_AbilityHotbar::SetAssignMode(bool bEnable)
{
	if (bAssignMode == bEnable) return;
	bAssignMode = bEnable;

	if (bAssignMode)
	{
		// 自身は透過にして、枠以外の場所のクリックは背後の割り当て画面へ通す（枠は個別にVisibleになる）
		VisibilityBeforeAssignMode = GetVisibility();
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		SetVisibility(VisibilityBeforeAssignMode);
	}

	// Z-Orderは追加時にしか指定できないため、付け直して割り当て画面（Z-Order 20）の手前／元の位置（0）へ移す。
	// 付け直すとNativeConstructで枠が作り直されるので、クリック可否の設定はその後に行う。
	if (IsInViewport())
	{
		RemoveFromParent();
		AddToViewport(bAssignMode ? 30 : 0);
	}

	for (UWBP_AbilitySlot* SlotWidget : Slots)
	{
		SlotWidget->SetClickable(bAssignMode);
	}
}

void UWBP_AbilityHotbar::HandleSlotClicked(int32 SlotIndex)
{
	OnSlotClicked.Broadcast(SlotIndex);
}
