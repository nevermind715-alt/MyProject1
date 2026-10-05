#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_ChestMenu.generated.h"

class UButton;
class UScrollBox;
class UWBP_ChestItemSlot;

/**
 * チェスト画面（中身の一覧と「閉じる」ボタン）のC++基底クラス。
 * 開閉はAMyProject1HUD::ToggleChestMenuが管理し、対象のチェストはHUDのCurrentChestCompから取る。
 * BP側（WBP_ChestMenu）のグラフは不要。
 */
UCLASS()
class MYPROJECT1_API UWBP_ChestMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一覧の各行に使うウィジェットクラス。クラスデフォルトで WBP_ChestItemSlot を指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest")
	TSubclassOf<UWBP_ChestItemSlot> ChestSlotClass;

	/** 開いているチェストの中身で一覧を作り直す */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	void RefreshList();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* ChestListScroll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Close;

private:
	UFUNCTION()
	void HandleCloseClicked();
};