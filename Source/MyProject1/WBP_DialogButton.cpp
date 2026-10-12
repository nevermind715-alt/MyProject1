#include "WBP_DialogButton.h"
#include "DialogComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UDialogComponent* FindPlayerDialogComponent(const UUserWidget* Widget)
	{
		const APawn* OwningPawn = Widget ? Widget->GetOwningPlayerPawn() : nullptr;
		return OwningPawn ? OwningPawn->FindComponentByClass<UDialogComponent>() : nullptr;
	}
}

void UWBP_DialogButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// NativeOnInitializedはウィジェット生成時に1回だけ呼ばれる
	if (Button_28)
	{
		Button_28->OnClicked.AddDynamic(this, &UWBP_DialogButton::HandleClicked);
	}
}

void UWBP_DialogButton::Setup(int32 InChoiceIndex, const FDialogChoice& InChoiceData)
{
	ChoiceIndex = InChoiceIndex;
	ChoiceData = InChoiceData;

	if (TextBlock_0)
	{
		TextBlock_0->SetText(FText::FromString(ChoiceData.ChoiceText));

		// 従来のBP（Switch on EDialogChoiceColor）と同じ色。白（通常）の文字色が黒なのは、ボタンの背景が明るい前提のため
		FLinearColor TextColor(0.0f, 0.0f, 0.0f, 1.0f);
		switch (ChoiceData.ChoiceColor)
		{
		case EDialogChoiceColor::White:
			TextColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		case EDialogChoiceColor::Red:
			TextColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
			break;
		case EDialogChoiceColor::Blue:
			TextColor = FLinearColor(0.0f, 0.08369f, 1.0f, 1.0f);
			break;
		case EDialogChoiceColor::Purple:
			TextColor = FLinearColor(1.0f, 0.0f, 0.772281f, 1.0f);
			break;
		}
		TextBlock_0->SetColorAndOpacity(FSlateColor(TextColor));
	}

	// 選択可否（RequiredStat/RequiredFlag/RequiredGilの条件）はUDialogComponentに判定させる
	if (const UDialogComponent* DialogComp = FindPlayerDialogComponent(this))
	{
		bCanSelectChoice = DialogComp->CanSelectChoice(ChoiceData);
	}
}

void UWBP_DialogButton::HandleClicked()
{
	if (!bCanSelectChoice)
	{
		if (ChoiceErrorSound)
		{
			UGameplayStatics::PlaySound2D(this, ChoiceErrorSound);
		}
		return;
	}

	if (UDialogComponent* DialogComp = FindPlayerDialogComponent(this))
	{
		DialogComp->SelectChoice(ChoiceIndex);
	}
}
