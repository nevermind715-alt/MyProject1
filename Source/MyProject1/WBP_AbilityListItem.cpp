#include "WBP_AbilityListItem.h"
#include "MyProject1Types.h"

void UWBP_AbilityListItem::NativeConstruct()
{
	Super::NativeConstruct();

	// NativeConstructはAddToViewport等のたびに呼ばれ得るため、AddDynamicの前にRemoveDynamicして二重バインドを防ぐ
	if (Btn_Select)
	{
		Btn_Select->OnClicked.RemoveDynamic(this, &UWBP_AbilityListItem::HandleSelectClicked);
		Btn_Select->OnClicked.AddDynamic(this, &UWBP_AbilityListItem::HandleSelectClicked);
	}
}

void UWBP_AbilityListItem::Setup(FName InAbilityID, const FAbilityData& Data)
{
	AbilityID = InAbilityID;

	if (Txt_Name)
	{
		Txt_Name->SetText(FText::FromString(Data.AbilityName));
	}

	if (Img_Icon)
	{
		if (Data.Icon)
		{
			Img_Icon->SetBrushFromTexture(Data.Icon);
			Img_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	SetAssignedSlot(-1);
	SetSelected(false);
}

void UWBP_AbilityListItem::SetAssignedSlot(int32 SlotIndex)
{
	if (!Txt_Assigned) return;

	if (SlotIndex < 0)
	{
		Txt_Assigned->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// インデックス0〜8がキー1〜9、9がキー0
	Txt_Assigned->SetText(FText::FromString(FString::Printf(TEXT("[%d]"), (SlotIndex + 1) % 10)));
	Txt_Assigned->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWBP_AbilityListItem::SetSelected(bool bSelected)
{
	if (Overlay_Selected)
	{
		Overlay_Selected->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWBP_AbilityListItem::HandleSelectClicked()
{
	OnItemClicked.Broadcast(AbilityID);
}
