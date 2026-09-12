#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "MyProject1Types.h"
#include "WBP_SaveSlot.generated.h"

// WBP_SaveMenuがボタン押下を受け取るための通知。引数はこの行が表示しているスロット名（SlotInfo.SlotName）。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveSlotActionRequested, const FString&, SlotName);

/**
 * セーブ画面の一覧1行分。表示更新（テキスト・表示/非表示・ボタン有効無効）は全てC++側で行うため、
 * BP側（WBP_SaveSlot）はDesignerでウィジェットを配置し、下記のBindWidget名に合わせるだけでよい。
 * グラフに実装すべきロジックは無い（BindWidgetOptionalなので未配置の項目があっても安全に動く）。
 */
UCLASS()
class MYPROJECT1_API UWBP_SaveSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	// このスロットが表示している内容。WBP_SaveMenuがセーブ後の再描画やクリック通知の引数元として参照する。
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FSaveSlotDisplayInfo SlotInfo;

	// WBP_SaveMenuから1件分の情報を渡して表示を更新させる
	UFUNCTION(BlueprintCallable, Category = "Save")
	void Setup(const FSaveSlotDisplayInfo& Info);

	// Btn_Saveが押された時にWBP_SaveMenu側でAddDynamicする通知
	UPROPERTY(BlueprintAssignable, Category = "Save")
	FOnSaveSlotActionRequested OnSaveClicked;

	// Btn_Loadが押された時にWBP_SaveMenu側でAddDynamicする通知
	UPROPERTY(BlueprintAssignable, Category = "Save")
	FOnSaveSlotActionRequested OnLoadClicked;

protected:
	virtual void NativeConstruct() override;

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	// "オートセーブ" / "スロット3" 等の見出し
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_SlotLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_PlayerLine;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_DateLine;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_SavedAt;

	// データが無いスロットの時だけ表示する「空き」表示
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Empty;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Save;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Load;

private:
	UFUNCTION()
	void HandleSaveButtonClicked();

	UFUNCTION()
	void HandleLoadButtonClicked();
};
