#include "WBP_AbilitySlot.h"
#include "AbilityComponent.h"
#include "MyProject1Character.h"

void UWBP_AbilitySlot::Setup(int32 InSlotIndex, bool bInClickable)
{
	SlotIndex = InSlotIndex;
	bClickable = bInClickable;

	// 枠の中身（SizeBox/Overlay等のパネル、Collapsed中のImage、HitTestInvisibleの文字）は
	// どれもクリックを受けないため、このウィジェット自身をVisibleにしておかないと
	// クリックが枠を素通りして NativeOnMouseButtonDown が呼ばれない
	if (bClickable)
	{
		SetVisibility(ESlateVisibility::Visible);
	}

	// インデックス0〜8がキー1〜9、9がキー0
	if (Txt_Key)
	{
		Txt_Key->SetText(FText::AsNumber((SlotIndex + 1) % 10));
	}
}

FReply UWBP_AbilitySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bClickable && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnSlotClicked.Broadcast(SlotIndex);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

UAbilityComponent* UWBP_AbilitySlot::GetAbilityComponent() const
{
	const AMyProject1Character* Character = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	return Character ? Character->AbilityComp : nullptr;
}

void UWBP_AbilitySlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// プレイヤーのPawnはウィジェット生成より後に確定することがあるため、取れない間は何もしない
	const UAbilityComponent* AbilityComp = GetAbilityComponent();
	if (!AbilityComp) return;

	const FName AbilityID = AbilityComp->HotbarSlots.IsValidIndex(SlotIndex) ? AbilityComp->HotbarSlots[SlotIndex] : NAME_None;

	if (!bIconInitialized || AbilityID != DisplayedAbilityID)
	{
		bIconInitialized = true;
		DisplayedAbilityID = AbilityID;
		RefreshIcon(*AbilityComp);
	}

	// クールタイム（リキャスト）の表示
	const float Remaining = AbilityID.IsNone() ? 0.0f : AbilityComp->GetRecastRemaining(AbilityID);
	const bool bCoolingDown = Remaining > 0.0f;

	if (Overlay_Cooldown)
	{
		Overlay_Cooldown->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Txt_Cooldown)
	{
		// 1秒以上は整数秒（切り上げ）、1秒未満は小数1桁
		FString CooldownText;
		if (bCoolingDown)
		{
			CooldownText = Remaining >= 1.0f
				? FString::FromInt(FMath::CeilToInt(Remaining))
				: FString::Printf(TEXT("%.1f"), Remaining);
		}

		if (CooldownText != DisplayedCooldownText)
		{
			DisplayedCooldownText = CooldownText;
			Txt_Cooldown->SetText(FText::FromString(CooldownText));
		}
		Txt_Cooldown->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWBP_AbilitySlot::RefreshIcon(const UAbilityComponent& AbilityComp)
{
	if (!Img_Icon) return;

	const FAbilityData* Data = AbilityComp.FindAbilityData(DisplayedAbilityID);
	if (Data && Data->Icon)
	{
		Img_Icon->SetBrushFromTexture(Data->Icon);
		Img_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		// 未割り当て、またはアイコン未設定のアビリティ
		Img_Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
}
