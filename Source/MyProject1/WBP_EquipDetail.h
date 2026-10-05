#pragma once

#include "CoreMinimal.h"
#include "WBP_ShopDetail.h"
#include "WBP_EquipDetail.generated.h"

class UTextBlock;

/**
 * 装備一覧の右側に出る「装備詳細」のC++基底クラス。親のUWBP_ShopDetailを継承。
 * アイテムの説明文を ItemDetails に表示し、ItemID が None / bVisible=false の時は隠す。
 */
UCLASS()
class MYPROJECT1_API UWBP_EquipDetail : public UWBP_ShopDetail
{
	GENERATED_BODY()

public:
	/** 詳細を更新する。装備一覧の行がホバーされた時に呼ばれる。 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|UI")
	void UpdateEquipDetail(FName ItemID, bool bVisible);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ItemDetails;
};