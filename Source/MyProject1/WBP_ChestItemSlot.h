#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_ChestItemSlot.generated.h"

class UButton;
class UTextBlock;
class UChestComponent;

/**
 * チェスト画面の1行（アイテム名・個数・「取り出す」ボタン）のC++基底クラス。
 * 生成する側（チェスト画面）が ItemID / Quantity / TargetChest を渡す（ExposeOnSpawn）。
 */
UCLASS()
class MYPROJECT1_API UWBP_ChestItemSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ExposeOnSpawn = "true"))
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ExposeOnSpawn = "true"))
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest", meta = (ExposeOnSpawn = "true"))
	UChestComponent* TargetChest = nullptr;

	/** 個数の表示を更新する */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	void UpdateQuantityText();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_ItemName;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Quantity;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Take;

private:
	UFUNCTION()
	void HandleTakeClicked();
};