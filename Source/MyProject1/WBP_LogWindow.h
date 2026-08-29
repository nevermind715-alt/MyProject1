#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_LogWindow.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_LogWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	// ログが1件追加されたらBP側からこれを呼ぶ（即表示させ、非表示までのタイマーをリセットする）
	UFUNCTION(BlueprintCallable, Category = "Log")
	void NotifyNewLogEntry();

	// 最後のログから何秒後に自動で非表示にするか
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	float AutoHideDelay = 25.0f;

protected:
	virtual void NativeConstruct() override;

	// BP側でフェードイン/スライドインのWidget Animationを再生する
	UFUNCTION(BlueprintImplementableEvent, Category = "Log")
	void OnShowLogWindow();

	// BP側でフェードアウト/スライドアウトのWidget Animationを再生する
	UFUNCTION(BlueprintImplementableEvent, Category = "Log")
	void OnHideLogWindow();

	// レベル移動後にこのウィンドウが新しく生成された時、NativeConstructから自動的に呼ばれる。
	// BP側で GameInstance の LogHistory をループし、AddLogEntry(bIsRestoring=true) で復元する処理を実装する。
	UFUNCTION(BlueprintImplementableEvent, Category = "Log")
	void RestoreLogHistory();

private:
	// 自動非表示タイマーが切れた時に呼ばれる
	void HandleAutoHideTimeout();

	// 現在ウィンドウが表示状態（フェードイン済み）かどうか。多重にShowアニメーションを再生してしまうのを防ぐ
	bool bIsWindowVisible = false;

	FTimerHandle AutoHideTimerHandle;
};
