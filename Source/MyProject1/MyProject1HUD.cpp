#include "MyProject1HUD.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "MyProject1Character.h"
#include "Kismet/GameplayStatics.h"
#include "ShopNPCBase.h"
#include "WBP_TimeSkipMenu.h"

namespace
{
	void SayShopGoodbye(AMyProject1Character* OwnerChar)
	{
		if (OwnerChar && OwnerChar->ActiveShopNPC)
		{
			FString FormattedGoodbye = FString::Printf(TEXT("%s : %s"), *OwnerChar->ActiveShopNPC->NPCName, *OwnerChar->ActiveShopNPC->GoodbyeText);
			OwnerChar->OnReceiveLogMessage(FormattedGoodbye, ELogMessageType::Dialogue);
			OwnerChar->ActiveShopNPC = nullptr;
		}
	}
}

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

    // アイテムショップが開いている時にメニューキーが押されたらショップを閉じる
    if (ItemShopMenuWidget && ItemShopMenuWidget->IsInViewport())
    {
        ToggleItemShopMenu();
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

void AMyProject1HUD::ForceCloseCommandMenuForInteract()
{
    // コマンドメニュー本体が開いている時だけ閉じる。
    // ToggleCommandMenu()は「開いていれば閉じる／閉じていれば開く」ので、
    // 開いている状態でだけ呼ぶことで強制クローズとして使える。
    if (CommandMenuWidget && CommandMenuWidget->IsInViewport())
    {
        ToggleCommandMenu();
    }
}

// --- 待機/睡眠メニュー ---
void AMyProject1HUD::OpenTimeSkipMenu(bool bIsSleepMode)
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC || !TimeSkipMenuClass) return;

    // 既に開いていれば多重生成しない
    if (TimeSkipMenuWidget && TimeSkipMenuWidget->IsInViewport()) return;

    // 他のメニューが開いていたら全て閉じる（操作不能バグ防止）。
    // 注意：ToggleCommandMenu()は「サブメニューが1つでも開いている間に呼ぶと、そのサブメニューだけ
    // 閉じてコマンドメニュー自体は表示したままにする」設計（Tabキーの1段階戻る動作）なので、
    // 先にコマンドメニューを閉じようとすると閉じきれない。必ず全サブメニューを閉じ切ってから、
    // 最後にCommandMenuWidget自体を閉じること（ToggleItemShopMenuと同じ並び順）。
    if (ActiveSubMenuWidget && ActiveSubMenuWidget->IsInViewport())
    {
        ActiveSubMenuWidget->RemoveFromParent();
        ActiveSubMenuWidget = nullptr;
    }
    if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport()) ToggleInventoryMenu();
    if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport()) ToggleEquipmentMenu();
    if (StatusMenuWidget && StatusMenuWidget->IsInViewport()) ToggleStatusMenu();
    if (QuestMenuWidget && QuestMenuWidget->IsInViewport()) ToggleQuestMenu();
    if (ChestMenuWidget && ChestMenuWidget->IsInViewport()) ToggleChestMenu();
    if (TreatmentMenuWidget && TreatmentMenuWidget->IsInViewport()) ToggleTreatmentMenu();
    if (TattooMenuWidget && TattooMenuWidget->IsInViewport()) ToggleTattooMenu();
    if (RestraintShopMenuWidget && RestraintShopMenuWidget->IsInViewport()) ToggleRestraintShopMenu();
    if (ItemShopMenuWidget && ItemShopMenuWidget->IsInViewport()) ToggleItemShopMenu();
    if (CommandMenuWidget && CommandMenuWidget->IsInViewport()) ToggleCommandMenu();

    TimeSkipMenuWidget = CreateWidget<UWBP_TimeSkipMenu>(PC, TimeSkipMenuClass);
    if (TimeSkipMenuWidget)
    {
        TimeSkipMenuWidget->bIsSleepMode = bIsSleepMode;
        TimeSkipMenuWidget->AddToViewport();
    }
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


void AMyProject1HUD::ToggleTreatmentMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return;

    if (!TreatmentMenuWidget && TreatmentMenuClass)
    {
        TreatmentMenuWidget = CreateWidget<UUserWidget>(GetWorld(), TreatmentMenuClass);
    }

    if (!TreatmentMenuWidget) return;

    if (!TreatmentMenuWidget->IsInViewport())
    {
        // 他の画面が開いていたら一斉に閉じる
        if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport()) ToggleInventoryMenu();
        if (CommandMenuWidget && CommandMenuWidget->IsInViewport()) ToggleCommandMenu();
        if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport()) ToggleEquipmentMenu();
        if (StatusMenuWidget && StatusMenuWidget->IsInViewport()) ToggleStatusMenu();
        if (QuestMenuWidget && QuestMenuWidget->IsInViewport()) ToggleQuestMenu();
        if (ChestMenuWidget && ChestMenuWidget->IsInViewport()) ChestMenuWidget->RemoveFromParent();

        // === 【追記】もし刺青専門店の画面が開いていたら自動で閉じる ===
        if (TattooMenuWidget && TattooMenuWidget->IsInViewport()) ToggleTattooMenu();

        TreatmentMenuWidget->AddToViewport(20);

        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(TreatmentMenuWidget->TakeWidget());
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(PC->GetPawn()))
        {
            OwnerChar->SetInputLocked(true);
        }

        if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
    }
    else
    {
        TreatmentMenuWidget->RemoveFromParent();

        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;

        if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(PC->GetPawn()))
        {
            OwnerChar->SetInputLocked(false);
            SayShopGoodbye(OwnerChar);
        }

        if (MenuCloseSound) UGameplayStatics::PlaySound2D(this, MenuCloseSound);
    }
}


void AMyProject1HUD::ToggleTattooMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return;

    // 1. 初回呼び出し時に刺青ショップ画面を動的生成
    if (!TattooMenuWidget && TattooMenuClass)
    {
        TattooMenuWidget = CreateWidget<UUserWidget>(GetWorld(), TattooMenuClass);
    }

    if (!TattooMenuWidget) return;

    // 2. トグル判定
    if (!TattooMenuWidget->IsInViewport())
    {
        // 【開く処理】他のメニューが開いていたらすべて閉じる（総合クリニック含む）
        if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport()) ToggleInventoryMenu();
        if (CommandMenuWidget && CommandMenuWidget->IsInViewport()) ToggleCommandMenu();
        if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport()) ToggleEquipmentMenu();
        if (StatusMenuWidget && StatusMenuWidget->IsInViewport()) ToggleStatusMenu();
        if (QuestMenuWidget && QuestMenuWidget->IsInViewport()) ToggleQuestMenu();
        if (ChestMenuWidget && ChestMenuWidget->IsInViewport()) ChestMenuWidget->RemoveFromParent();

        // 総合クリニックが開いていたら閉じる
        if (TreatmentMenuWidget && TreatmentMenuWidget->IsInViewport()) ToggleTreatmentMenu();

        // 画面の最前面（Z-Order 20）へ配置
        TattooMenuWidget->AddToViewport(20);

        // マウス解放・入力フォーカスの固定
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(TattooMenuWidget->TakeWidget());
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        // キャラクターの移動・旋回ロック
        if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(PC->GetPawn()))
        {
            OwnerChar->SetInputLocked(true);
        }

        // 開封用システムSE
        if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
    }
    else
    {
        // 【閉じる処理】画面から撤去
        TattooMenuWidget->RemoveFromParent();
        CurrentNPCShopIDs.Empty();

        // 通常フィールド操作（ゲームオンリー）に入力を引き戻す
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;

        // キャラクターの操作ロック解除
        if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(PC->GetPawn()))
        {
            OwnerChar->SetInputLocked(false);
            SayShopGoodbye(OwnerChar);
        }

        // クローズ用システムSE
        if (MenuCloseSound) UGameplayStatics::PlaySound2D(this, MenuCloseSound);
    }
}

// 解錠屋メニューの開閉。ToggleTattooMenu と同じ構造（別ウィジェットクラス・別インスタンス変数）。
void AMyProject1HUD::ToggleRestraintShopMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return;

    // 1. 初回呼び出し時に解錠屋ショップ画面を動的生成
    if (!RestraintShopMenuWidget && RestraintShopMenuClass)
    {
        RestraintShopMenuWidget = CreateWidget<UUserWidget>(GetWorld(), RestraintShopMenuClass);
    }

    if (!RestraintShopMenuWidget) return;

    // 2. トグル判定
    if (!RestraintShopMenuWidget->IsInViewport())
    {
        // 【開く処理】他のメニューが開いていたらすべて閉じる
        if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport()) ToggleInventoryMenu();
        if (CommandMenuWidget && CommandMenuWidget->IsInViewport()) ToggleCommandMenu();
        if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport()) ToggleEquipmentMenu();
        if (StatusMenuWidget && StatusMenuWidget->IsInViewport()) ToggleStatusMenu();
        if (QuestMenuWidget && QuestMenuWidget->IsInViewport()) ToggleQuestMenu();
        if (ChestMenuWidget && ChestMenuWidget->IsInViewport()) ChestMenuWidget->RemoveFromParent();
        if (TreatmentMenuWidget && TreatmentMenuWidget->IsInViewport()) ToggleTreatmentMenu();
        if (TattooMenuWidget && TattooMenuWidget->IsInViewport()) ToggleTattooMenu();

        // 画面の最前面（Z-Order 20）へ配置
        RestraintShopMenuWidget->AddToViewport(20);

        // マウス解放・入力フォーカスの固定
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(RestraintShopMenuWidget->TakeWidget());
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        // キャラクターの移動・旋回ロック
        if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(PC->GetPawn()))
        {
            OwnerChar->SetInputLocked(true);
        }

        if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
    }
    else
    {
        // 【閉じる処理】画面から撤去
        RestraintShopMenuWidget->RemoveFromParent();

        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;

        if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(PC->GetPawn()))
        {
            OwnerChar->SetInputLocked(false);
            SayShopGoodbye(OwnerChar);
        }

        if (MenuCloseSound) UGameplayStatics::PlaySound2D(this, MenuCloseSound);
    }
}

void AMyProject1HUD::ToggleItemShopMenu()
{
    APlayerController* PC = GetOwningPlayerController();
    if (!PC) return;

    AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(PC->GetPawn());
    if (!PlayerChar) return;

    // 1. 【閉じる処理】すでにショップ画面が開いているなら、画面から撤去する
    if (ItemShopMenuWidget && ItemShopMenuWidget->IsInViewport())
    {
        ItemShopMenuWidget->RemoveFromParent();

        // WBP_ItemShopはOnInventoryUpdated/OnItemHoverChangedにBindしたままRemoveFromParent()されるため、
        // 閉じてもデリゲートに参照が残り続け、閉店後のアイテム使用等でUpdateShopListが誤って再実行されてしまう。
        // ここで明示的に解除する（RemoveAllはバインドされていなくても安全）。
        if (UInventoryComponent* Inv = PlayerChar->FindComponentByClass<UInventoryComponent>())
        {
            Inv->OnInventoryUpdated.RemoveAll(ItemShopMenuWidget);
            Inv->OnItemHoverChanged.RemoveAll(ItemShopMenuWidget);
        }

        // フィールド操作に入力を引き戻す（ゲームオンリー・操作ロック解除）
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
        PlayerChar->SetInputLocked(false);
        SayShopGoodbye(PlayerChar);

        if (MenuCloseSound) UGameplayStatics::PlaySound2D(this, MenuCloseSound);
        return;
    }

    // 2. まだ作られていなければ生成する (他のメニューと合わせて GetWorld() に統一)
    if (!ItemShopMenuWidget && ItemShopMenuClass)
    {
        ItemShopMenuWidget = CreateWidget<UUserWidget>(GetWorld(), ItemShopMenuClass);
    }

    // 3. 【開く処理】画面を表示する
    if (ItemShopMenuWidget)
    {
        // もし他のUIが開いていたら操作不能バグ防止のため全て閉じる
        if (InventoryMenuWidget && InventoryMenuWidget->IsInViewport()) ToggleInventoryMenu();
        if (CommandMenuWidget && CommandMenuWidget->IsInViewport()) ToggleCommandMenu();
        if (EquipmentMenuWidget && EquipmentMenuWidget->IsInViewport()) ToggleEquipmentMenu();
        if (StatusMenuWidget && StatusMenuWidget->IsInViewport()) ToggleStatusMenu();
        if (QuestMenuWidget && QuestMenuWidget->IsInViewport()) ToggleQuestMenu();
        if (ChestMenuWidget && ChestMenuWidget->IsInViewport()) ChestMenuWidget->RemoveFromParent();
        if (TreatmentMenuWidget && TreatmentMenuWidget->IsInViewport()) ToggleTreatmentMenu();
        if (TattooMenuWidget && TattooMenuWidget->IsInViewport()) ToggleTattooMenu();

        // 画面の手前（Z-Order 20）へ配置
        ItemShopMenuWidget->AddToViewport(20);

        // 入力フォーカスの固定とマウスカーソルの表示、キャラクターの移動ロック
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(ItemShopMenuWidget->TakeWidget());
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
        PlayerChar->SetInputLocked(true);

        if (MenuOpenSound) UGameplayStatics::PlaySound2D(this, MenuOpenSound);
    }
}