#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_TimeSkipMenu.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_TimeSkipMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** trueなら睡眠モード（起床時刻を選ぶ）、falseなら待機モード（経過時間を選ぶ）。
	 *  HUD側のOpenTimeSkipMenuが生成直後にセットする */
	UPROPERTY(BlueprintReadOnly, Category = "Time Skip")
	bool bIsSleepMode = false;

	/** 待機モードのスライダーの最大時間（時間単位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Skip")
	float MaxWaitHours = 72.0f;

	/** 睡眠モードのスライダーの最大時間（時間単位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Skip")
	float MaxSleepHours = 12.0f;

	/** bIsSleepModeに応じて、今のモードで使うべきスライダーの最大時間を返す。
	 *  BP側はOnTimeSkipMenuReadyの中でスライダーのMax Valueにこの値をセットするだけでよい */
	UFUNCTION(BlueprintPure, Category = "Time Skip")
	float GetMaxTimeSkipHours() const { return bIsSleepMode ? MaxSleepHours : MaxWaitHours; }

	/** 待機モード時のボタン表記 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Skip")
	FText WaitActionText = FText::FromString(TEXT("待機します"));

	/** 睡眠モード時のボタン表記 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Skip")
	FText SleepActionText = FText::FromString(TEXT("寝ます"));

	/** bIsSleepModeに応じて、確定ボタン等に出すべき表記を返す。
	 *  BP側はOnTimeSkipMenuReadyの中でテキストのSet TextにこれをセットするかText Bindingで使う */
	UFUNCTION(BlueprintPure, Category = "Time Skip")
	FText GetConfirmActionText() const { return bIsSleepMode ? SleepActionText : WaitActionText; }

	/** 待機：指定した時間（時間単位）だけその場で時間を進める */
	UFUNCTION(BlueprintCallable, Category = "Time Skip")
	void ConfirmWaitHours(int32 Hours);

	/** 睡眠：指定した時刻（0〜23）まで時間を進めて起床する */
	UFUNCTION(BlueprintCallable, Category = "Time Skip")
	void ConfirmSleepUntilHour(int32 TargetHour);

	/** 睡眠：指定した時間（時間単位）だけ眠る（疲労度はFatigueDecreasePercentPerSleepHourに従って回復する） */
	UFUNCTION(BlueprintCallable, Category = "Time Skip")
	void ConfirmSleepHours(int32 Hours);

	/** メニューを閉じる（マウスカーソルと入力モードを元に戻し、ビューポートから外す） */
	UFUNCTION(BlueprintCallable, Category = "Time Skip")
	void CloseTimeSkipMenu();

protected:
	virtual void NativeConstruct() override;

	/** ウィジェット生成直後、bIsSleepModeが確定した状態で呼ばれる。
	 *  BP側はこれを受けて待機用/睡眠用のボタンレイアウトを出し分ける */
	UFUNCTION(BlueprintImplementableEvent, Category = "Time Skip")
	void OnTimeSkipMenuReady(bool bInIsSleepMode);
};
