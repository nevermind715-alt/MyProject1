#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "MyProject1Types.h"
#include "WBP_DialogButton.generated.h"

class USoundBase;

/**
 * 会話の選択肢ボタン1個分のC++基底クラス（BPアセット名は WBP_DialogButon）。
 * UWBP_ChoiceMenu::RefreshChoicesが生成してSetupを呼ぶ。テキスト・選択可否・色分け・クリック時の
 * UDialogComponent::SelectChoice呼び出しまで全てここで行うため、BP側はDesignerでの見た目作りと
 * BindWidget名合わせ・ChoiceErrorSoundの指定だけでよく、グラフに実装すべきロジックは無い。
 */
UCLASS()
class MYPROJECT1_API UWBP_DialogButton : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 選択肢のインデックスとデータを受け取り、テキスト・選択可否・文字色を反映する（CreateWidget直後、AddChildの前に呼ぶ） */
	void Setup(int32 InChoiceIndex, const FDialogChoice& InChoiceData);

	/** 選べない選択肢（条件を満たさない）をクリックした時に鳴らす音。BP側のクラスデフォルトで指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog|Audio")
	USoundBase* ChoiceErrorSound;

protected:
	virtual void NativeOnInitialized() override;

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TextBlock_0;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Button_28;

private:
	int32 ChoiceIndex = 0;
	FDialogChoice ChoiceData;
	bool bCanSelectChoice = false;

	UFUNCTION()
	void HandleClicked();
};
