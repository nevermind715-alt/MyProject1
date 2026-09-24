#include "WBP_SaveMenu.h"
#include "WBP_SaveSlot.h"
#include "MyProject1GameInstance.h"
#include "MyProject1HUD.h"
#include "GameFramework/PlayerController.h"

void UWBP_SaveMenu::NativeConstruct()
{
	Super::NativeConstruct();

	// NativeConstructはAddToViewportのたびに毎回呼ばれる（オブジェクト生成時の1回だけではない）。
	// SaveMenuWidgetはHUD側で使い回される（開閉のたびに作り直されない）ため、
	// AddDynamicの前にRemoveDynamicしておかないと、開閉を繰り返すたびにバインドが積み重なり、
	// 1クリックでハンドラが複数回呼ばれてしまう（閉じてもすぐ開き直る等の不具合の原因になる）。
	if (Btn_Close)
	{
		Btn_Close->OnClicked.RemoveDynamic(this, &UWBP_SaveMenu::HandleCloseClicked);
		Btn_Close->OnClicked.AddDynamic(this, &UWBP_SaveMenu::HandleCloseClicked);
	}
	if (Btn_ConfirmYes)
	{
		Btn_ConfirmYes->OnClicked.RemoveDynamic(this, &UWBP_SaveMenu::HandleConfirmYesClicked);
		Btn_ConfirmYes->OnClicked.AddDynamic(this, &UWBP_SaveMenu::HandleConfirmYesClicked);
	}
	if (Btn_ConfirmNo)
	{
		Btn_ConfirmNo->OnClicked.RemoveDynamic(this, &UWBP_SaveMenu::HandleConfirmNoClicked);
		Btn_ConfirmNo->OnClicked.AddDynamic(this, &UWBP_SaveMenu::HandleConfirmNoClicked);
	}

	HideConfirmDialog();
	RefreshSlotList();

	OnMenuOpened();
}

void UWBP_SaveMenu::RefreshSlotList()
{
	if (!Scroll_Slots || !SaveSlotWidgetClass) return;

	Scroll_Slots->ClearChildren();
	SlotWidgetsBySlotName.Empty();

	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	for (const FSaveSlotDisplayInfo& Info : GameInst->GetAllSaveSlotInfos())
	{
		UWBP_SaveSlot* Row = CreateWidget<UWBP_SaveSlot>(this, SaveSlotWidgetClass);
		if (!Row) continue;

		Row->Setup(Info);
		Row->OnSaveClicked.AddDynamic(this, &UWBP_SaveMenu::HandleSaveRequested);
		Row->OnLoadClicked.AddDynamic(this, &UWBP_SaveMenu::HandleLoadRequested);

		Scroll_Slots->AddChild(Row);
		SlotWidgetsBySlotName.Add(Info.SlotName, Row);
	}
}

void UWBP_SaveMenu::HandleSaveRequested(const FString& SlotName)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	// オートセーブ枠へは手動セーブさせない（WBP_SaveSlot::SetupがBtn_Saveを隠すが、念のため二重に防ぐ）
	if (SlotName == UMyProject1GameInstance::AutoSaveSlotName) return;

	if (GameInst->DoesSaveGameExist(SlotName))
	{
		ShowConfirmDialog(NSLOCTEXT("WBP_SaveMenu", "OverwriteConfirm", "このスロットに上書きしますか？"), SlotName, /*bIsLoad=*/false);
	}
	else
	{
		ExecuteSaveToSlot(SlotName);
	}
}

void UWBP_SaveMenu::HandleLoadRequested(const FString& SlotName)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst || !GameInst->DoesSaveGameExist(SlotName)) return;

	ShowConfirmDialog(NSLOCTEXT("WBP_SaveMenu", "LoadConfirm", "このデータをロードします。よろしいですか？"), SlotName, /*bIsLoad=*/true);
}

void UWBP_SaveMenu::ShowConfirmDialog(const FText& Message, const FString& SlotName, bool bIsLoad)
{
	PendingSlotName = SlotName;
	bPendingIsLoad = bIsLoad;

	if (Txt_ConfirmMsg) Txt_ConfirmMsg->SetText(Message);
	if (Overlay_Confirm) Overlay_Confirm->SetVisibility(ESlateVisibility::Visible);
}

void UWBP_SaveMenu::HideConfirmDialog()
{
	if (Overlay_Confirm) Overlay_Confirm->SetVisibility(ESlateVisibility::Collapsed);
}

void UWBP_SaveMenu::HandleConfirmYesClicked()
{
	HideConfirmDialog();

	if (bPendingIsLoad)
	{
		ExecuteLoadFromSlot(PendingSlotName);
	}
	else
	{
		ExecuteSaveToSlot(PendingSlotName);
	}
}

void UWBP_SaveMenu::HandleConfirmNoClicked()
{
	HideConfirmDialog();
}

void UWBP_SaveMenu::ExecuteSaveToSlot(const FString& SlotName)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	if (GameInst->SaveCurrentGame(SlotName))
	{
		// 一覧を作り直さず、書き込んだ行だけ最新情報で差し替える（チラつき防止）
		if (UWBP_SaveSlot** Row = SlotWidgetsBySlotName.Find(SlotName))
		{
			if (*Row)
			{
				(*Row)->Setup(GameInst->GetSaveSlotInfo(SlotName));
			}
		}
	}
}

void UWBP_SaveMenu::ExecuteLoadFromSlot(const FString& SlotName)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst || !GameInst->DoesSaveGameExist(SlotName)) return;

	// LoadSavedGame内で即座にOpenLevelされる。開いたままのSaveMenu/CommandMenuを閉じずに
	// レベル遷移すると、ポーズ状態・入力モード・マウスカーソル表示が中途半端なまま新レベルへ
	// 持ち越され、「メインメニューが消えて操作が混ざる」不具合になるため、遷移前に正規の手順で閉じる
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AMyProject1HUD* HUD = PC->GetHUD<AMyProject1HUD>())
		{
			// このセーブ画面を通常の閉じ方で閉じる（ポーズ解除・プレイヤー入力再開を含む）
			HUD->ToggleSaveMenu();

			// 背後のメインメニュー（CommandMenu）が開いたままなら、それも閉じてゲーム操作に戻す
			HUD->ForceCloseCommandMenuForInteract();
		}
	}

	// この画面自体の後始末は上記で完了しているため、あとはレベル遷移に任せてよい
	GameInst->LoadSavedGame(SlotName);
}

void UWBP_SaveMenu::HandleCloseClicked()
{
	CloseSaveMenu();
}

void UWBP_SaveMenu::CloseSaveMenu()
{
	// 開くときと同じHUDのトグルを呼んで、背後のコマンドメニューへ操作フォーカスを戻す
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AMyProject1HUD* HUD = PC->GetHUD<AMyProject1HUD>())
		{
			HUD->ToggleSaveMenu();
			return;
		}
	}

	// HUDが取れない万一のフォールバック
	RemoveFromParent();
}
