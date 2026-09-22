#include "WBP_AnimSequenceDebugMenu.h"
#include "MyProject1GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

void UWBP_AnimSequenceDebugMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		OnAnimSequenceListReady(GameInst->GetAllAnimSequenceRows());
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

void UWBP_AnimSequenceDebugMenu::PlayDebugRow(FName RowName)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	ACharacter* PlayerChar = Cast<ACharacter>(GetOwningPlayerPawn());
	if (GameInst && PlayerChar)
	{
		GameInst->PlayAnimSequenceRowDirect(RowName, PlayerChar, nullptr, EStatTargetActor::Player);
	}

	// 再生中はメニューが画面に残って見づらいため、WarpDebugMenuと同様に再生と同時に閉じる。
	// 別の行を試す時はデバッグキーで開き直す
	CloseDebugMenu();
}

void UWBP_AnimSequenceDebugMenu::CloseDebugMenu()
{
	// マウスカーソルと入力モードを、メニューを開く前の通常のゲーム操作状態に戻す
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	RemoveFromParent();
}
