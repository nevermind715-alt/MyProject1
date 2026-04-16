#include "MyProject1HUD.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "MyProject1Character.h"
#include "Kismet/GameplayStatics.h"

void AMyProject1HUD::BeginPlay()
{
	Super::BeginPlay();

	// エディタ側でクラスが設定されているか確認
	if (PlayerStatusWidgetClass)
	{
		// ウィジェットを生成
		PlayerStatusWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerStatusWidgetClass);

		if (PlayerStatusWidget)
		{
			// 画面に表示
			PlayerStatusWidget->AddToViewport();
		}
	}
}

void AMyProject1HUD::ToggleCommandMenu()
{
    // もしインベントリメニューが存在し、画面に表示されているなら
    if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport())
    {
        // 「インベントリを閉じる処理」を呼び出す
        // (ToggleInventoryMenu関数の中に「閉じたらコマンドメニューを表示する」処理が書いてあるため)
        ToggleInventoryMenu();

        // ここで return して、下の「コマンドメニューを消す処理」に行かせない
        return;
    }

    // もしクエストメニューが存在し、画面に表示されているなら
    if (QuestMenuWidget && QuestMenuWidget->IsInViewport())
    {
        ToggleQuestMenu();
        return;
    }

    APlayerController* PC = GetOwningPlayerController();
    

    if (!PC || !CommandMenuClass) return;

    if (!CommandMenuWidget)
    {
        CommandMenuWidget = CreateWidget<UUserWidget>(GetWorld(), CommandMenuClass);
    }

    if (CommandMenuWidget)
    {
        if (!CommandMenuWidget->IsInViewport())
        {
            CommandMenuWidget->SetVisibility(ESlateVisibility::Visible);
            CommandMenuWidget->AddToViewport(10);
            FInputModeGameAndUI InputMode;
            PC->bShowMouseCursor = true;
            InputMode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(InputMode);

            
            if (MenuOpenSound)
            {
                UGameplayStatics::PlaySound2D(this, MenuOpenSound);
            }
        }
        else
        {
            if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport())
            {
                InventoryMenuWidget->RemoveFromParent();
            }

            CommandMenuWidget->RemoveFromParent();

            // 通常のゲーム操作に戻す
            FInputModeGameOnly InputMode;
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = false;

           
            if (MenuCloseSound)
            {
                UGameplayStatics::PlaySound2D(this, MenuCloseSound);
            }
        }
    }
}

// --- インベントリ（メニュー内のボタンから開く方） ---
void AMyProject1HUD::ToggleInventoryMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !InventoryMenuClass) return;

    if (!InventoryMenuWidget)
    {
        InventoryMenuWidget = CreateWidget<UUserWidget>(GetWorld(), InventoryMenuClass);
    }

    if (InventoryMenuWidget)
    {
        if (!InventoryMenuWidget->IsInViewport())
        {
            InventoryMenuWidget->AddToViewport(20);

            // 背後のメニューを隠す
            if (CommandMenuWidget) CommandMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(InventoryMenuWidget->TakeWidget());
            PC->SetInputMode(InputMode);

            if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
        }
        else
        {
            InventoryMenuWidget->RemoveFromParent();

            // --- 修正：メニューを再表示する際に確実に Visible にする ---
            if (CommandMenuWidget)
            {
                CommandMenuWidget->SetVisibility(ESlateVisibility::Visible);

                FInputModeGameAndUI InputMode;
                InputMode.SetWidgetToFocus(CommandMenuWidget->TakeWidget());
                PC->SetInputMode(InputMode);

                if (MenuCloseSound)
                {
                    UGameplayStatics::PlaySound2D(this, MenuCloseSound);
                }
            }
        }
    }
}

bool AMyProject1HUD::IsCommandMenuOpen() const
{
    // ウィジェットが存在し、かつ画面に表示されているなら true を返す
    return CommandMenuWidget && CommandMenuWidget->IsInViewport();
}

// --- クエスト（メニュー内のボタンから開く方） ---
void AMyProject1HUD::ToggleQuestMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !QuestMenuClass) return;

    if (!QuestMenuWidget)
    {
        QuestMenuWidget = CreateWidget<UUserWidget>(GetWorld(), QuestMenuClass);
    }

    if (QuestMenuWidget)
    {
        if (!QuestMenuWidget->IsInViewport())
        {
            // Z-Orderを20にして手前に表示
            QuestMenuWidget->AddToViewport(20);

            // 背後のメニューを隠す
            if (CommandMenuWidget) CommandMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

            // 入力フォーカスをクエスト画面に向ける
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(QuestMenuWidget->TakeWidget());
            PC->SetInputMode(InputMode);

            if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
        }
        else
        {
            // 閉じる処理
            QuestMenuWidget->RemoveFromParent();

            // 背後のメニューを再表示する
            if (CommandMenuWidget)
            {
                CommandMenuWidget->SetVisibility(ESlateVisibility::Visible);

                FInputModeGameAndUI InputMode;
                InputMode.SetWidgetToFocus(CommandMenuWidget->TakeWidget());
                PC->SetInputMode(InputMode);

                if (MenuCloseSound)
                {
                    UGameplayStatics::PlaySound2D(this, MenuCloseSound);
                }
            }
        }
    }
}