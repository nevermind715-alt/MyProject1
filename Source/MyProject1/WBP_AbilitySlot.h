#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "WBP_AbilitySlot.generated.h"

class UAbilityComponent;

// 割り当て画面（WBP_AbilityMenu）がスロットのクリックを受け取るための通知。引数はこの枠のスロット番号（0=キー1 ... 9=キー0）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilitySlotClicked, int32, SlotIndex);

/**
 * ホットバー（1〜0キー）の枠1つ分のC++基底クラス。
 * 割り当てられたアビリティのアイコン、キー番号、クールタイム（暗転＋残り秒数）を毎フレーム最新の状態に保つ。
 * BP側（WBP_AbilitySlot）はDesignerでの見た目作りと、下記のBindWidget名合わせだけでよい。グラフに実装すべきロジックは無い。
 */
UCLASS()
class MYPROJECT1_API UWBP_AbilitySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * どのスロット（0=キー1 ... 9=キー0）の枠か。生成した側が最初に1回呼ぶ。
	 * bInClickableがtrueの枠だけ、左クリックでOnSlotClickedを通知する（割り当て画面用。画面上のホットバーはfalse）。
	 */
	void Setup(int32 InSlotIndex, bool bInClickable = false);

	/** クリックされた時にWBP_AbilityMenu側でAddDynamicする通知（bInClickable=trueの枠のみ） */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FOnAbilitySlotClicked OnSlotClicked;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	/** アビリティのアイコン。未割り当てのスロットでは非表示になる */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Img_Icon;

	/** キー番号の表示（1〜9, 0） */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Key;

	/** クールタイム中に枠を暗くする半透明の板（Image/Border等、Visibilityを切り替えられれば何でも良い） */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Overlay_Cooldown;

	/** クールタイムの残り秒数 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Cooldown;

private:
	UAbilityComponent* GetAbilityComponent() const;

	/** アイコン表示を、現在のスロット内容に合わせる */
	void RefreshIcon(const UAbilityComponent& AbilityComp);

	int32 SlotIndex = 0;
	bool bClickable = false;

	/** 最後に反映したアビリティID。変化した時だけアイコンを差し替えるための記憶 */
	FName DisplayedAbilityID;
	bool bIconInitialized = false;

	/** 最後に反映したクールタイム表示。変化した時だけSetTextするための記憶 */
	FString DisplayedCooldownText;
};
