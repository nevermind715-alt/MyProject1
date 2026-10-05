#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_EquipItemRow.generated.h"

class UButton;
class UTextBlock;
class UWBP_EquipItemList;
class UWBP_EquipDetail;

/**
 * 装備一覧（WBP_EquipItemList）の1行。クリックで装備、ホバーで右側の詳細を更新する。
 */
UCLASS()
class MYPROJECT1_API UWBP_EquipItemRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一覧が生成直後に呼ぶ。表示内容と参照を渡す。 */
	void SetupRow(FName InItemID, UWBP_EquipItemList* InParentList, UWBP_EquipDetail* InDetail, bool bInEquipped);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_SelectItem;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_ItemName;

private:
	UFUNCTION()
	void HandleSelectClicked();

	void RefreshName();

	FName ItemID;
	bool bEquipped = false;

	UPROPERTY()
	UWBP_EquipItemList* ParentList = nullptr;

	UPROPERTY()
	UWBP_EquipDetail* DetailWidget = nullptr;
};