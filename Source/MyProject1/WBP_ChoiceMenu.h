#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "MyProject1Types.h"
#include "WBP_ChoiceMenu.generated.h"

class UWBP_DialogButton;
class USoundBase;

/**
 * 会話の選択肢一覧のC++基底クラス。
 * 生成・表示・入力モードの切り替えはUDialogComponentが行い、ここでは選択肢ボタンの並べ直し（RefreshChoices）と
 * 表示の遅延・効果音までを受け持つ。BP側（WBP_ChoiceMenu）はDesignerでの見た目作りと、BindWidget名合わせ・
 * ChoiceButtonClass／効果音の指定だけでよく、グラフに実装すべきロジックは無い。
 */
UCLASS()
class MYPROJECT1_API UWBP_ChoiceMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 選択肢ボタンを作り直して並べる。いったん隠し、UDialogComponent::ChoiceRevealDelay秒後に表示する。 */
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void RefreshChoices(const TArray<FDialogChoice>& InChoices);

	/** 選択肢を隠す（選んだ直後。表示待ちのタイマーも止める） */
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void HideChoices();

	/** 選択肢1個分のウィジェットクラス。BP側のクラスデフォルトでWBP_DialogButonを指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	TSubclassOf<UWBP_DialogButton> ChoiceButtonClass;

	/** 選択肢を並べ終えた瞬間に鳴らす音（従来はインタラクト音を指定していた）。BP側のクラスデフォルトで指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Audio")
	USoundBase* ChoicesListedSound;

	/** 選択肢が表示された瞬間に鳴らす音（従来はCoins_03_Cueを指定していた）。BP側のクラスデフォルトで指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Audio")
	USoundBase* ChoicesRevealedSound;

protected:
	virtual void NativeDestruct() override;

	/** 選択肢ボタンを入れる縦並びの箱。WBP側の変数名と完全一致させる（BindWidgetOptionalなので未配置でも安全に動く） */
	UPROPERTY(meta = (BindWidgetOptional))
	UVerticalBox* ChoiceBox;

private:
	FTimerHandle RevealTimerHandle;

	void HandleRevealTimer();
};
