#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "WBP_AbilityHotbar.generated.h"

class UWBP_AbilitySlot;

// 割り当てモード中にホットバーの枠がクリックされた通知。引数はスロット番号（0=キー1 ... 9=キー0）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHotbarSlotClicked, int32, SlotIndex);

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

	/**
	 * 割り当てモードの切替。アビリティ割り当て画面（WBP_AbilityMenu）が開いている間だけtrueにする。
	 * trueの間は、画面上のこのホットバーを割り当て画面より手前に出し、各枠をクリック可能にする（クリックでOnSlotClickedを通知）。
	 * falseで元の表示（Z-Order 0・クリック不可）へ戻す。
	 */
	void SetAssignMode(bool bEnable);

	/** 割り当てモード中に枠がクリックされた通知（WBP_AbilityMenu側でAddDynamicする） */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FOnHotbarSlotClicked OnSlotClicked;

protected:
	virtual void NativeConstruct() override;

	/** UniformGridPanelを使う場合の1行あたりの枠数（これを超えると下の行へ折り返す） */
	static constexpr int32 SlotsPerRow = 6;

	/** 枠を並べる箱（HorizontalBox等。UniformGridPanelなら6個ごとに折り返す。それ以外はAddChildで順に並べる） */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* Box_Slots;

private:
	UFUNCTION()
	void HandleSlotClicked(int32 SlotIndex);

	/** 生成した枠。UPROPERTYで保持（GC対策）。NativeConstructのたびに作り直される */
	UPROPERTY()
	TArray<UWBP_AbilitySlot*> Slots;

	bool bAssignMode = false;

	/** 割り当てモードに入る前のこのウィジェット自身のVisibility（戻す時に復元する） */
	ESlateVisibility VisibilityBeforeAssignMode = ESlateVisibility::SelfHitTestInvisible;
};
