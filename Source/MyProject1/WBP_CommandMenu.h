#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "WBP_CommandMenu.generated.h"

/**
 * コマンドメニュー（WBP_CommondMenu）のC++基底クラス。
 * 各ボタンの押下→HUDのToggleXxxMenu/キャラのToggleCombatMode呼び出しと、
 * 戦闘ボタンの文字・各ボタンの有効/無効（死亡中は押せない）までを全てここで行う。
 * BP側（WBP_CommondMenu）はDesignerでの見た目作りと、下記のBindWidget名合わせだけでよい。
 * グラフ・バインド関数に実装すべきロジックは無い。
 */
UCLASS()
class MYPROJECT1_API UWBP_CommandMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Stats;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Save;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Quest;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Item;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Equ;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Attack;

	/** アビリティボタン。押すとアビリティ割り当てメニューを開く */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Abl;

	/** 戦闘ボタンの文字（「戦闘開始」/「戦闘停止」）。名前を付けていない場合はBtn_Attackの子のTextBlockを自動で探す */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Fight;

private:
	UFUNCTION()
	void HandleStatsClicked();

	UFUNCTION()
	void HandleSaveClicked();

	UFUNCTION()
	void HandleQuestClicked();

	UFUNCTION()
	void HandleItemClicked();

	UFUNCTION()
	void HandleEquClicked();

	UFUNCTION()
	void HandleAttackClicked();

	UFUNCTION()
	void HandleAbilityClicked();

	/** 戦闘ボタンの文字と、各ボタンの有効/無効を現在のキャラ状態に合わせる */
	void RefreshState();
};
