#include "WBP_ChoiceMenu.h"
#include "WBP_DialogButton.h"
#include "DialogComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UWBP_ChoiceMenu::RefreshChoices(const TArray<FDialogChoice>& InChoices)
{
	// どちらもBP（WBP_ChoiceMenu）側の設定項目。未設定だと選択肢が出ず会話が進まなくなるため、原因が分かるようログを出す
	if (!ChoiceBox || !ChoiceButtonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialog] WBP_ChoiceMenu: ChoiceBox（Designerの縦並びの箱）または ChoiceButtonClass が未設定のため、選択肢を表示できません。"));
		return;
	}

	ChoiceBox->ClearChildren();
	SetVisibility(ESlateVisibility::Collapsed);

	int32 ChoiceIndex = 0;
	for (const FDialogChoice& Choice : InChoices)
	{
		UWBP_DialogButton* ChoiceButton = CreateWidget<UWBP_DialogButton>(GetOwningPlayer(), ChoiceButtonClass);
		if (ChoiceButton)
		{
			ChoiceButton->Setup(ChoiceIndex, Choice);
			ChoiceBox->AddChildToVerticalBox(ChoiceButton);
		}
		++ChoiceIndex;
	}

	if (ChoicesListedSound)
	{
		UGameplayStatics::PlaySound2D(this, ChoicesListedSound);
	}

	// セリフ表示と同時に選択肢が出ると不自然なため、UDialogComponent::ChoiceRevealDelay秒待ってから表示する
	float RevealDelay = 0.5f;
	if (const APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (const UDialogComponent* DialogComp = OwningPawn->FindComponentByClass<UDialogComponent>())
		{
			RevealDelay = DialogComp->ChoiceRevealDelay;
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RevealTimerHandle, this, &UWBP_ChoiceMenu::HandleRevealTimer, FMath::Max(RevealDelay, 0.01f), false);
	}
}

void UWBP_ChoiceMenu::HandleRevealTimer()
{
	SetVisibility(ESlateVisibility::Visible);

	if (ChoicesRevealedSound)
	{
		UGameplayStatics::PlaySound2D(this, ChoicesRevealedSound);
	}
}

void UWBP_ChoiceMenu::HideChoices()
{
	// 表示待ちの間に選択された（または会話が閉じた）場合、後から選択肢が現れないようにタイマーも止める
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevealTimerHandle);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_ChoiceMenu::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevealTimerHandle);
	}

	Super::NativeDestruct();
}
