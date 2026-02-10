#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1Types.h"
#include "WBP_ShopDetail.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_ShopDetail : public UUserWidget
{
	GENERATED_BODY()

protected:
	// Widgetが生成された時に呼ばれる（BPのConstruct相当）
	virtual void NativeConstruct() override;

	// 放送（デリゲート）を受け取った時に実行する関数
	UFUNCTION()
	void HandleHoverChanged(FName NewItemID);

	// ★ロジックはC++、見た目（テキストのセットなど）はBPでやるためのイベント
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void OnUpdateDetailDisplay(const FItemData& ItemData, bool bIsVisible);
};