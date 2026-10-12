#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "WBP_FadeNarration.generated.h"

/**
 * 暗転中ナレーション（ActionType=ShowTextDuringFade、イベント開始/終了ナレーション、睡眠ナレーション）の
 * 1行表示用ウィジェットのC++基底クラス。
 * UDialogComponent::OnFadeNarrationLine/OnFadeNarrationClosedを受けたUDialogComponent自身が生成・表示・
 * 行の更新を行うため、BP側（WBP_FadeNarration）はDesignerでの見た目作りとBindWidget名合わせだけでよい。
 * グラフに実装すべきロジックは無い（UDialogComponentのデリゲートをBPで購読する必要も無い）。
 */
UCLASS()
class MYPROJECT1_API UWBP_FadeNarration : public UUserWidget
{
	GENERATED_BODY()

public:
	/** ナレーションの1行を表示する（テキストを差し替えてVisibleにする） */
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void ShowLine(const FText& Line);

	/** ナレーション表示を隠す（全行読み終えた時） */
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void HideNarration();

protected:
	/** ナレーション本文のTextBlock。WBP側の変数名と完全一致させる（BindWidgetOptionalなので未配置でも安全に動く） */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_FadeNarration;
};
