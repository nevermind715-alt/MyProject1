#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "WBP_AbilityHotbar.generated.h"

class UWBP_AbilitySlot;

/**
 * 画面に常時出す、アビリティのホットバー（1〜0キーの10枠）のC++基底クラス。
 * 枠の生成・キー番号・アイコン・クールタイム表示は全てC++側（UWBP_AbilitySlot）で行う。
 * BP側（WBP_AbilityHotbar）はDesignerでの配置（Box_Slotsを置く）と、SlotWidgetClassの指定だけでよい。グラフに実装すべきロジックは無い。
 * 表示はAMyProject1HUDのBeginPlayで行う（AbilityHotbarWidgetClassにこのBPを指定する）。
 */
UCLASS()
class MYPROJECT1_API UWBP_AbilityHotbar : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 枠1つ分に使うウィジェットクラス。BP側のクラスデフォルトでWBP_AbilitySlotを指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSubclassOf<UWBP_AbilitySlot> SlotWidgetClass;

protected:
	virtual void NativeConstruct() override;

	/** 枠を並べる箱（HorizontalBox等。UPanelWidgetのAddChild/ClearChildrenだけ使う） */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* Box_Slots;
};
