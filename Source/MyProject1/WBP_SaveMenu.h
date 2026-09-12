#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "MyProject1Types.h"
#include "WBP_SaveMenu.generated.h"

class UWBP_SaveSlot;

/**
 * セーブ／ロード兼用画面のC++基底クラス。
 * 開閉は他のサブメニュー（ステータス／装備／クエスト）と同じくAMyProject1HUD::ToggleSaveMenuが管理する。
 * 一覧生成・上書き確認・ロード確認・セーブ／ロードの実行・行の再描画まで全てここで行うため、
 * BP側（WBP_SaveMenu）はDesignerでの見た目作りと、下記のBindWidget名合わせ・SaveSlotWidgetClassの指定だけでよい。
 * グラフに実装すべきロジックは無い（OnMenuOpenedで演出を足したい場合のみ任意で実装する）。
 */
UCLASS()
class MYPROJECT1_API UWBP_SaveMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一覧の各行に使うウィジェットクラス。BP側のクラスデフォルトでWBP_SaveSlotを指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	TSubclassOf<UWBP_SaveSlot> SaveSlotWidgetClass;

	/** 一覧を最新のセーブ状況で作り直す。開いた時に自動で呼ばれるが、外から強制更新したい時にも使える。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void RefreshSlotList();

	/** 閉じる要求。HUDのToggleSaveMenuへ中継し、背後のコマンドメニューへ操作を戻す。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void CloseSaveMenu();

protected:
	virtual void NativeConstruct() override;

	/** 一覧生成が終わった直後に呼ばれる。BP側は演出（フェードイン等）を足したい場合だけ実装すればよい（任意）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save")
	void OnMenuOpened();

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	/** 一覧の行を入れる箱（ScrollBox/VerticalBoxどちらでも良い。UPanelWidgetのAddChild/ClearChildrenだけ使う） */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* Scroll_Slots;

	/** 上書き/ロードの確認ダイアログ全体（Overlay/Border等、Visibilityを切り替えられれば何でも良い） */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Overlay_Confirm;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_ConfirmMsg;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ConfirmYes;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ConfirmNo;

private:
	// スロット名→行ウィジェット。セーブ直後にその行だけ差し替えるためのルックアップ。
	UPROPERTY()
	TMap<FString, UWBP_SaveSlot*> SlotWidgetsBySlotName;

	// 確認ダイアログで「はい」を押した時に、どのスロットへ何をするかの記憶
	FString PendingSlotName;
	bool bPendingIsLoad = false;

	UFUNCTION()
	void HandleSaveRequested(const FString& SlotName);

	UFUNCTION()
	void HandleLoadRequested(const FString& SlotName);

	UFUNCTION()
	void HandleConfirmYesClicked();

	UFUNCTION()
	void HandleConfirmNoClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void ExecuteSaveToSlot(const FString& SlotName);
	void ExecuteLoadFromSlot(const FString& SlotName);

	void ShowConfirmDialog(const FText& Message, const FString& SlotName, bool bIsLoad);
	void HideConfirmDialog();
};
