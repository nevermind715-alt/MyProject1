#include "WBP_TimeSkipMenu.h"
#include "MyProject1GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

void UWBP_TimeSkipMenu::NativeConstruct()
{
	Super::NativeConstruct();

	OnTimeSkipMenuReady(bIsSleepMode);

	// メニュー表示中はマウス操作でボタンを選べるようにする
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void UWBP_TimeSkipMenu::ConfirmWaitHours(int32 Hours)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	ACharacter* PlayerChar = Cast<ACharacter>(GetOwningPlayerPawn());
	if (GameInst && PlayerChar)
	{
		GameInst->RequestFadeThenAdvanceTime(FMath::Max(Hours, 0) * 60, PlayerChar);
	}

	CloseTimeSkipMenu();
}

void UWBP_TimeSkipMenu::ConfirmSleepUntilHour(int32 TargetHour)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	ACharacter* PlayerChar = Cast<ACharacter>(GetOwningPlayerPawn());
	if (GameInst && PlayerChar)
	{
		TargetHour = FMath::Clamp(TargetHour, 0, 23);
		const int32 TargetMinutes = TargetHour * 60;

		// 現在時刻から目標時刻までの残り分数（必ず「今より未来」になるよう1440分でラップする）
		int32 MinutesToAdd = ((TargetMinutes - GameInst->CurrentTimeInMinutes) % 1440 + 1440) % 1440;

		// ちょうど今と同じ時刻を選んだ場合は1日分寝たことにする（0分待機を防ぐ）
		if (MinutesToAdd == 0)
		{
			MinutesToAdd = 1440;
		}

		GameInst->RequestFadeThenAdvanceTime(MinutesToAdd, PlayerChar, /*bIsSleep=*/true);
	}

	CloseTimeSkipMenu();
}

void UWBP_TimeSkipMenu::ConfirmSleepHours(int32 Hours)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	ACharacter* PlayerChar = Cast<ACharacter>(GetOwningPlayerPawn());
	if (GameInst && PlayerChar)
	{
		GameInst->RequestFadeThenAdvanceTime(FMath::Max(Hours, 0) * 60, PlayerChar, /*bIsSleep=*/true);
	}

	CloseTimeSkipMenu();
}

void UWBP_TimeSkipMenu::CloseTimeSkipMenu()
{
	// マウスカーソルと入力モードを、メニューを開く前の通常のゲーム操作状態に戻す
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	RemoveFromParent();
}
