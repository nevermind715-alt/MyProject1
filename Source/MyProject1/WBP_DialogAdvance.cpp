#include "WBP_DialogAdvance.h"
#include "DialogComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

UDialogComponent* UWBP_DialogAdvance::FindDialogComponent() const
{
	const APawn* OwningPawn = GetOwningPlayerPawn();
	return OwningPawn ? OwningPawn->FindComponentByClass<UDialogComponent>() : nullptr;
}

void UWBP_DialogAdvance::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// NativeOnInitializedはウィジェット生成時に1回だけ呼ばれる（NativeConstructのようにAddToViewportのたびには呼ばれない）
	if (Button_0)
	{
		Button_0->OnPressed.AddDynamic(this, &UWBP_DialogAdvance::HandleButtonPressed);
	}
}

void UWBP_DialogAdvance::NativeConstruct()
{
	Super::NativeConstruct();

	RequestAdvanceFocus();
	UpdateAdvanceVisibility();
}

FReply UWBP_DialogAdvance::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		RequestAdvance();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWBP_DialogAdvance::HandleButtonPressed()
{
	RequestAdvance();
}

void UWBP_DialogAdvance::RequestAdvance()
{
	if (UDialogComponent* DialogComp = FindDialogComponent())
	{
		DialogComp->AdvanceDialog();
	}
}

void UWBP_DialogAdvance::UpdateAdvanceVisibility()
{
	const UDialogComponent* DialogComp = FindDialogComponent();
	const bool bChoicesActive = DialogComp && DialogComp->AreChoicesActive();
	SetVisibility(bChoicesActive ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UWBP_DialogAdvance::RequestAdvanceFocus()
{
	SetKeyboardFocus();
}
