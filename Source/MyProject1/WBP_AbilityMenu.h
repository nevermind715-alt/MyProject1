#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "WBP_AbilityMenu.generated.h"

class UAbilityComponent;
class UWBP_AbilityListItem;
class UWBP_AbilitySlot;

/**
 * アビリティ割り当て画面のC++基底クラス。習得済みアビリティの一覧から1つ選び、ホットバー（1〜0キー）のスロットをクリックして割り当てる。
 * 開閉は他のサブメニュー（ステータス／装備／クエスト／セーブ）と同じくAMyProject1HUD::ToggleAbilityMenuが管理する。
 * 一覧・スロット枠の生成、選択、割り当て、解除、説明表示まで全てここで行うため、
 * BP側（WBP_AbilityMenu）はDesignerでの見た目作りと、下記のBindWidget名合わせ・ListItemClass/HotbarSlotClassの指定だけでよい。
 * グラフに実装すべきロジックは無い。
 *
 * 操作：
 *  - 一覧の行をクリック → そのアビリティを選択（もう一度クリックで選択解除）
 *  - 選択中にスロット枠をクリック → そのスロットへ割り当て（選択は解除される）
 *  - 一覧・スロット枠以外の場所（何もない所）をクリック → 選択を解除
 *  - 何も選択していない時に、割り当て済みのスロット枠をクリック → そのスロットを空にする
 */
UCLASS()
class MYPROJECT1_API UWBP_AbilityMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一覧の各行に使うウィジェットクラス。BP側のクラスデフォルトでWBP_AbilityListItemを指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSubclassOf<UWBP_AbilityListItem> ListItemClass;

	/** 下側に並べるスロット枠に使うウィジェットクラス。BP側のクラスデフォルトでWBP_AbilitySlot（ホットバーと同じもの）を指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSubclassOf<UWBP_AbilitySlot> HotbarSlotClass;

	/** 閉じる要求。HUDのToggleAbilityMenuへ中継し、背後のコマンドメニューへ操作を戻す。 */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void CloseAbilityMenu();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 一覧の行・スロット枠・ボタン以外の場所（何もない所）をクリックしたら、アビリティの選択を解除する */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	/** 習得済みアビリティの行を入れる箱（ScrollBox/VerticalBoxどちらでも良い。UPanelWidgetのAddChild/ClearChildrenだけ使う） */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* Scroll_Abilities;

	/** スロット枠（10個）を並べる箱（HorizontalBox等） */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* Box_HotbarSlots;

	/** 選択中アビリティの名前・説明・数値 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_SelectedName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Detail;

	/** 操作の案内文 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Hint;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Close;

private:
	UAbilityComponent* GetAbilityComponent() const;

	/** 習得済みアビリティの一覧とスロット枠を作り直す */
	void RebuildLists();

	/** 各行の「[1]」等の割り当て表示と選択の強調を、現在の状態に合わせる（行は作り直さない） */
	void RefreshListMarks();

	/** 選択中アビリティの説明と案内文を更新する */
	void RefreshDetail();

	UFUNCTION()
	void HandleListItemClicked(FName AbilityID);

	UFUNCTION()
	void HandleHotbarSlotClicked(int32 SlotIndex);

	UFUNCTION()
	void HandleHotbarChanged();

	UFUNCTION()
	void HandleCloseClicked();

	/** 一覧の行。UPROPERTYで保持（GC対策） */
	UPROPERTY()
	TArray<UWBP_AbilityListItem*> ListItems;

	/** 現在選択中のアビリティ（未選択はNAME_None） */
	FName SelectedAbilityID;
};
