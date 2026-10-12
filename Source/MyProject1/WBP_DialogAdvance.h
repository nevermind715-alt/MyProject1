#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "WBP_DialogAdvance.generated.h"

class UDialogComponent;

/**
 * 「クリック／スペースで会話を進める」全画面ウィジェットのC++基底クラス。
 * 生成・表示・入力モード・削除は全てUDialogComponentが行う（セリフに次の行がある時、または暗転ナレーションの開始時に
 * 1つだけ作り、会話終了で消す）。BP側（WBP_DialogAdvance）はDesignerでの見た目作りとBindWidget名合わせだけでよく、
 * グラフに実装すべきロジックは無い。
 * 注意：クラス設定（Class Settings）の「Is Focusable」はONのままにしておくこと（スペースキー入力を受けるため）。
 */
UCLASS()
class MYPROJECT1_API UWBP_DialogAdvance : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 選択肢の入力待ち中は隠し（背後の選択肢ボタンへクリックを通す）、それ以外はVisibleにする */
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void UpdateAdvanceVisibility();

	/** キーボードフォーカスを自分へ戻す（スペースキーを受けるため） */
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void RequestAdvanceFocus();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 画面全体を覆う透明ボタン。WBP側の変数名と完全一致させる（BindWidgetOptionalなので未配置でも安全に動く） */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Button_0;

private:
	/** プレイヤーのDialogComponentを都度取得する（PlayerRefをキャッシュしない） */
	UDialogComponent* FindDialogComponent() const;

	void RequestAdvance();

	UFUNCTION()
	void HandleButtonPressed();
};
