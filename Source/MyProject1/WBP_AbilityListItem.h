#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "WBP_AbilityListItem.generated.h"

struct FAbilityData;

// WBP_AbilityMenuが行のクリックを受け取るための通知。引数はこの行のアビリティID。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityListItemClicked, FName, AbilityID);

/**
 * アビリティ割り当て画面の「習得済みアビリティ一覧」1行分。表示更新は全てC++側で行うため、
 * BP側（WBP_AbilityListItem）はDesignerでウィジェットを配置し、下記のBindWidget名に合わせるだけでよい。
 * グラフに実装すべきロジックは無い（BindWidgetOptionalなので未配置の項目があっても安全に動く）。
 */
UCLASS()
class MYPROJECT1_API UWBP_AbilityListItem : public UUserWidget
{
	GENERATED_BODY()

public:
	/** この行が表すアビリティID。WBP_AbilityMenuが参照する */
	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	FName AbilityID;

	/** WBP_AbilityMenuから1件分の情報を渡して表示を更新させる */
	void Setup(FName InAbilityID, const FAbilityData& Data);

	/** ホットバーのどのスロットに入っているかを表示する（-1なら未割り当てで非表示） */
	void SetAssignedSlot(int32 SlotIndex);

	/** 選択中の強調表示を切り替える */
	void SetSelected(bool bSelected);

	/** 行がクリックされた時にWBP_AbilityMenu側でAddDynamicする通知 */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FOnAbilityListItemClicked OnItemClicked;

protected:
	virtual void NativeConstruct() override;

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Img_Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Name;

	/** 割り当て済みのキー番号（"[1]" 等）。未割り当てでは非表示 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Assigned;

	/** 選択中の行だけ表示する強調（Image/Border等、Visibilityを切り替えられれば何でも良い） */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Overlay_Selected;

	/** 行全体を覆うクリック用ボタン */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Select;

private:
	UFUNCTION()
	void HandleSelectClicked();
};
