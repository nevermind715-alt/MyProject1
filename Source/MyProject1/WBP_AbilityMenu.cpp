#include "WBP_AbilityMenu.h"
#include "WBP_AbilityListItem.h"
#include "WBP_AbilitySlot.h"
#include "AbilityComponent.h"
#include "MyProject1Character.h"
#include "MyProject1HUD.h"
#include "GameFramework/PlayerController.h"

void UWBP_AbilityMenu::NativeConstruct()
{
	Super::NativeConstruct();

	// NativeConstructはAddToViewportのたびに毎回呼ばれる（ウィジェットはHUD側で使い回される）ため、
	// AddDynamicの前にRemoveDynamicしておかないと、開閉のたびにバインドが積み重なってしまう。
	if (Btn_Close)
	{
		Btn_Close->OnClicked.RemoveDynamic(this, &UWBP_AbilityMenu::HandleCloseClicked);
		Btn_Close->OnClicked.AddDynamic(this, &UWBP_AbilityMenu::HandleCloseClicked);
	}

	if (UAbilityComponent* AbilityComp = GetAbilityComponent())
	{
		AbilityComp->OnHotbarChanged.RemoveDynamic(this, &UWBP_AbilityMenu::HandleHotbarChanged);
		AbilityComp->OnHotbarChanged.AddDynamic(this, &UWBP_AbilityMenu::HandleHotbarChanged);
	}

	// 何もない所のクリックを受け取れるよう、自身をVisibleにする（パネル類は初期設定でクリックを受けないため、
	// 背景に何も置いていない場所のクリックは素通りしてNativeOnMouseButtonDownが呼ばれない）
	SetVisibility(ESlateVisibility::Visible);

	SelectedAbilityID = NAME_None;
	RebuildLists();
	RefreshDetail();
}

FReply UWBP_AbilityMenu::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 行のボタンとスロット枠はクリックを処理済みにするため、ここに来るのはそれ以外の場所のクリックだけ
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !SelectedAbilityID.IsNone())
	{
		SelectedAbilityID = NAME_None;
		RefreshListMarks();
		RefreshDetail();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWBP_AbilityMenu::NativeDestruct()
{
	if (UAbilityComponent* AbilityComp = GetAbilityComponent())
	{
		AbilityComp->OnHotbarChanged.RemoveDynamic(this, &UWBP_AbilityMenu::HandleHotbarChanged);
	}

	Super::NativeDestruct();
}

UAbilityComponent* UWBP_AbilityMenu::GetAbilityComponent() const
{
	const AMyProject1Character* Character = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	return Character ? Character->AbilityComp : nullptr;
}

void UWBP_AbilityMenu::RebuildLists()
{
	UAbilityComponent* AbilityComp = GetAbilityComponent();
	if (!AbilityComp) return;

	// 習得済みアビリティの一覧
	ListItems.Empty();
	if (Scroll_Abilities && ListItemClass)
	{
		Scroll_Abilities->ClearChildren();

		for (const FName AbilityID : AbilityComp->LearnedAbilities)
		{
			const FAbilityData* Data = AbilityComp->FindAbilityData(AbilityID);
			if (!Data)
			{
				// 習得IDがデータテーブルに無い（行名の誤り・削除）。黙って消さず、原因が追えるようログを残す
				UE_LOG(LogTemp, Warning, TEXT("WBP_AbilityMenu: 習得済みのアビリティ '%s' がAbilityDataTableに見つかりません"), *AbilityID.ToString());
				continue;
			}

			UWBP_AbilityListItem* Row = CreateWidget<UWBP_AbilityListItem>(this, ListItemClass);
			if (!Row) continue;

			Row->Setup(AbilityID, *Data);
			Row->OnItemClicked.AddDynamic(this, &UWBP_AbilityMenu::HandleListItemClicked);

			Scroll_Abilities->AddChild(Row);
			ListItems.Add(Row);
		}
	}

	// ホットバーと同じ10個のスロット枠（こちらはクリックで割り当て/解除ができる）
	if (Box_HotbarSlots && HotbarSlotClass)
	{
		Box_HotbarSlots->ClearChildren();

		for (int32 SlotIndex = 0; SlotIndex < UAbilityComponent::NumHotbarSlots; ++SlotIndex)
		{
			UWBP_AbilitySlot* SlotWidget = CreateWidget<UWBP_AbilitySlot>(this, HotbarSlotClass);
			if (!SlotWidget) continue;

			SlotWidget->Setup(SlotIndex, /*bInClickable=*/true);
			SlotWidget->OnSlotClicked.AddDynamic(this, &UWBP_AbilityMenu::HandleHotbarSlotClicked);

			Box_HotbarSlots->AddChild(SlotWidget);
		}
	}

	RefreshListMarks();
}

void UWBP_AbilityMenu::RefreshListMarks()
{
	const UAbilityComponent* AbilityComp = GetAbilityComponent();
	if (!AbilityComp) return;

	for (UWBP_AbilityListItem* Row : ListItems)
	{
		// 入っていなければINDEX_NONE(-1)になり、SetAssignedSlot側で非表示になる
		Row->SetAssignedSlot(AbilityComp->HotbarSlots.IndexOfByKey(Row->AbilityID));
		Row->SetSelected(!SelectedAbilityID.IsNone() && Row->AbilityID == SelectedAbilityID);
	}
}

void UWBP_AbilityMenu::RefreshDetail()
{
	const UAbilityComponent* AbilityComp = GetAbilityComponent();
	const FAbilityData* Data = AbilityComp ? AbilityComp->FindAbilityData(SelectedAbilityID) : nullptr;

	if (!Data)
	{
		// 未選択
		if (Txt_SelectedName) Txt_SelectedName->SetText(FText::GetEmpty());
		if (Txt_Description) Txt_Description->SetText(FText::GetEmpty());
		if (Txt_Detail) Txt_Detail->SetText(FText::GetEmpty());
		if (Txt_Hint)
		{
			Txt_Hint->SetText(NSLOCTEXT("WBP_AbilityMenu", "HintNone",
				"アビリティを選んでから、割り当てたいスロットをクリックしてください。\n何も選んでいない状態で割り当て済みのスロットをクリックすると、そのスロットを外します。"));
		}
		return;
	}

	if (Txt_SelectedName) Txt_SelectedName->SetText(FText::FromString(Data->AbilityName));
	if (Txt_Description) Txt_Description->SetText(Data->Description);
	if (Txt_Detail)
	{
		Txt_Detail->SetText(FText::FromString(FString::Printf(
			TEXT("スタミナ %.0f / TP %d / 詠唱 %.1f秒 / リキャスト %.1f秒"),
			Data->CostStamina, Data->CostTP, Data->CastTime, Data->RecastTime)));
	}
	if (Txt_Hint)
	{
		Txt_Hint->SetText(FText::Format(
			NSLOCTEXT("WBP_AbilityMenu", "HintSelected", "「{0}」を選択中。割り当てたいスロットをクリックしてください。\n（何もない所をクリックすると選択を解除します。解除してから割り当て済みのスロットをクリックすると、そのスロットを外せます）"),
			FText::FromString(Data->AbilityName)));
	}
}

void UWBP_AbilityMenu::HandleListItemClicked(FName AbilityID)
{
	// 選択中の行をもう一度押したら選択解除
	SelectedAbilityID = (SelectedAbilityID == AbilityID) ? NAME_None : AbilityID;

	RefreshListMarks();
	RefreshDetail();
}

void UWBP_AbilityMenu::HandleHotbarSlotClicked(int32 SlotIndex)
{
	UAbilityComponent* AbilityComp = GetAbilityComponent();
	if (!AbilityComp) return;

	if (!SelectedAbilityID.IsNone())
	{
		// 割り当てが成功したら選択を解除する（続けて別のスロットを誤って上書きしないため）
		if (AbilityComp->AssignHotbarSlot(SlotIndex, SelectedAbilityID))
		{
			SelectedAbilityID = NAME_None;
		}
	}
	else if (AbilityComp->HotbarSlots.IsValidIndex(SlotIndex) && !AbilityComp->HotbarSlots[SlotIndex].IsNone())
	{
		AbilityComp->ClearHotbarSlot(SlotIndex);
	}

	RefreshListMarks();
	RefreshDetail();
}

void UWBP_AbilityMenu::HandleHotbarChanged()
{
	RefreshListMarks();
}

void UWBP_AbilityMenu::HandleCloseClicked()
{
	CloseAbilityMenu();
}

void UWBP_AbilityMenu::CloseAbilityMenu()
{
	// 開くときと同じHUDのトグルを呼んで、背後のコマンドメニューへ操作フォーカスを戻す
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AMyProject1HUD* HUD = PC->GetHUD<AMyProject1HUD>())
		{
			HUD->ToggleAbilityMenu();
			return;
		}
	}

	// HUDが取れない万一のフォールバック
	RemoveFromParent();
}
