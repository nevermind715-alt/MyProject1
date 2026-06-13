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
    if (ActiveSubMenuWidget && ActiveSubMenuWidget->IsInViewport())
    {
        ActiveSubMenuWidget->RemoveFromParent();
        ActiveSubMenuWidget = nullptr; // 記憶をクリア

        // サブメニューが消えたので、親である「装備メインメニュー」に操作フォーカスを戻す
        APlayerController* PC = GetOwningPlayerController();
        if (PC && EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport())
        {
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(EquipmentMenuWidget->TakeWidget());
            PC->SetInputMode(InputMode);
        }

        if (MenuCloseSound) UGameplayStatics::PlaySound2D(this, MenuCloseSound);
        return; // ここで処理を終了させることで、後ろの「EquipmentMenuWidgetを消す処理」に行かせない！
    }

    if (ChestMenuWidget && ChestMenuWidget->IsInViewport())
    {
        ToggleChestMenu();
        return;
    }

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

    // もし装備メニューが存在し、画面に表示されているなら
    if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport())
    {
        ToggleEquipmentMenu();
        return;
    }

    // もしステータスメニューが存在し、画面に表示されているなら
    if (StatusMenuWidget && StatusMenuWidget->IsInViewport())
    {
        ToggleStatusMenu();
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

// --- 装備（メニュー内のボタンから開く方） ---
void AMyProject1HUD::ToggleEquipmentMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !EquipmentMenuClass) return;

    if (!EquipmentMenuWidget)
    {
        // まだ作られていなければ生成する
        EquipmentMenuWidget = CreateWidget<UUserWidget>(GetWorld(), EquipmentMenuClass);
    }

    if (EquipmentMenuWidget)
    {
        if (!EquipmentMenuWidget->IsInViewport())
        {
            // Z-Orderを20にして手前に表示
            EquipmentMenuWidget->AddToViewport(20);

            // 背後のメニュー（WBP_CommandMenu）を隠す
            if (CommandMenuWidget) CommandMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

            // 入力フォーカスを装備画面に向ける
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(EquipmentMenuWidget->TakeWidget());
            PC->SetInputMode(InputMode);

            if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
        }
        else
        {
            // 閉じる処理
            EquipmentMenuWidget->RemoveFromParent();

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

// ---ステータスメニューの開閉処理 ---
void AMyProject1HUD::ToggleStatusMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !StatusMenuClass) return;

    if (!StatusMenuWidget)
    {
        // まだ作られていなければ生成する
        StatusMenuWidget = CreateWidget<UUserWidget>(GetWorld(), StatusMenuClass);
    }

    if (StatusMenuWidget)
    {
        if (!StatusMenuWidget->IsInViewport())
        {
            // Z-Orderを20にして手前に表示
            StatusMenuWidget->AddToViewport(20);

            // 背後のメニューを隠す
            if (CommandMenuWidget) CommandMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

            // 入力フォーカスをステータス画面に向ける
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(StatusMenuWidget->TakeWidget());
            PC->SetInputMode(InputMode);

            if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
        }
        else
        {
            // 閉じる処理
            StatusMenuWidget->RemoveFromParent();

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

// --- チェストメニューの開閉処理 ---
void AMyProject1HUD::ToggleChestMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !ChestMenuClass) return;

    if (!ChestMenuWidget)
    {
        // まだ作られていなければ生成する
        ChestMenuWidget = CreateWidget<UUserWidget>(GetWorld(), ChestMenuClass);
    }

    if (ChestMenuWidget)
    {
        if (!ChestMenuWidget->IsInViewport())
        {
            // ★排他制御：もし他のメニューが開いていたら全て閉じる（操作不能バグ防止）
            if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport()) ToggleInventoryMenu();
            if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport()) ToggleEquipmentMenu();
            if (StatusMenuWidget && StatusMenuWidget->IsInViewport()) ToggleStatusMenu();
            if (QuestMenuWidget && QuestMenuWidget->IsInViewport()) ToggleQuestMenu();

            // Z-Orderを20にして手前に表示
            ChestMenuWidget->AddToViewport(20);

            // 背後のメニュー（WBP_CommandMenu等）を念のため隠す
            if (CommandMenuWidget) CommandMenuWidget->SetVisibility(ESlateVisibility::Collapsed);

            // 入力フォーカスをチェスト画面に向け、マウスカーソルを表示
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(ChestMenuWidget->TakeWidget());
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;

            if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
        }
        else
        {
            // 閉じる処理
            ChestMenuWidget->RemoveFromParent();

            // 閉じたので、記憶していたチェストをクリア
            CurrentChestComp = nullptr;

            // フィールド操作に戻す（ゲームオンリー・マウス非表示）
            FInputModeGameOnly InputMode;
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = false;

            // 背後のコマンドメニューを再表示可能にする
            if (CommandMenuWidget)
            {
                CommandMenuWidget->SetVisibility(ESlateVisibility::Visible);
            }

            if (MenuCloseSound)
            {
                UGameplayStatics::PlaySound2D(this, MenuCloseSound);
            }
        }
    }
}