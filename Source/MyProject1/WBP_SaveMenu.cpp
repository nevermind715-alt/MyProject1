#include "WBP_SaveMenu.h"
#include "MyProject1GameInstance.h"
#include "MyProject1HUD.h"
#include "GameFramework/PlayerController.h"

void UWBP_SaveMenu::NativeConstruct()
{
	Super::NativeConstruct();

	// 入力モード／マウスカーソルはAMyProject1HUD::ToggleSaveMenu側で設定済み（他サブメニューと同じ流儀）。
	// ここでは初期のスロット一覧をBPへ渡すだけ。
	OnSlotListReady(GetSlotList());
}

TArray<FSaveSlotDisplayInfo> UWBP_SaveMenu::GetSlotList() const
{
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		return GameInst->GetAllSaveSlotInfos();
	}
	return TArray<FSaveSlotDisplayInfo>();
}

void UWBP_SaveMenu::ExecuteSaveToSlot(const FString& SlotName)
{
	if (SlotName.IsEmpty()) return;

	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	// オートセーブ専用スロットへは手動セーブさせない（一覧では読み取り専用の想定）
	if (SlotName == UMyProject1GameInstance::AutoSaveSlotName) return;

	if (GameInst->SaveCurrentGame(SlotName))
	{
		OnSlotSaved(GameInst->GetSaveSlotInfo(SlotName));
	}
}

void UWBP_SaveMenu::ExecuteLoadFromSlot(const FString& SlotName)
{
	if (SlotName.IsEmpty()) return;

	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	// 空きスロットのロードは無視（BP側でもボタンを無効化しておくこと）
	if (!GameInst->DoesSaveGameExist(SlotName)) return;

	// LoadSavedGame内でOpenLevelされるため、この画面や入力モードの後始末はレベル遷移に任せてよい
	GameInst->LoadSavedGame(SlotName);
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
