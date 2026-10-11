#include "TitleGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

ATitleGameMode::ATitleGameMode()
{
	// タイトルではプレイヤーキャラを出さない
	DefaultPawnClass = nullptr;
}

void ATitleGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !TitleMenuClass) return;

	if (UUserWidget* Title = CreateWidget<UUserWidget>(PC, TitleMenuClass))
	{
		Title->AddToViewport();
	}
}
