#include "WBP_WarpDebugMenu.h"
#include "MyProject1GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

void UWBP_WarpDebugMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		OnWarpListReady(GameInst->GetAllWarpDestinations());
	}

	// デバッグメニュー表示中はマウス操作でボタンを選べるようにする
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
}

void UWBP_WarpDebugMenu::WarpToDebugDestination(FName WarpID)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	ACharacter* PlayerChar = Cast<ACharacter>(GetOwningPlayerPawn());
	if (GameInst && PlayerChar)
	{
		GameInst->RequestWarp(WarpID, PlayerChar, /*bBypassRequiredFlag=*/true);
	}

	CloseDebugMenu();
}

void UWBP_WarpDebugMenu::CloseDebugMenu()
{
	// マウスカーソルと入力モードを、メニューを開く前の通常のゲーム操作状態に戻す
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	RemoveFromParent();
}
