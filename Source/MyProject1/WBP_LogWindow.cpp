#include "WBP_LogWindow.h"
#include "TimerManager.h"

void UWBP_LogWindow::NativeConstruct()
{
	Super::NativeConstruct();

	// 初回ログが来るまでは非表示状態から始める（さもないとデザイナー上の初期Opacityのまま表示され続ける）
	SetRenderOpacity(0.0f);
}

void UWBP_LogWindow::NotifyNewLogEntry()
{
	// まだ非表示の時だけフェードインを再生する。
	// 表示中に毎回ここでアニメーションを再生し直すと、Opacityが一瞬アニメーションの再生開始位置の値へ
	// スナップしてから戻るため、ログが出るたびにウィンドウが点滅して見えてしまう。
	if (!bIsWindowVisible)
	{
		bIsWindowVisible = true;

		// NativeConstructでルートのRenderOpacityを0にしているため、表示開始時に1へ戻す。
		// FadeLogWindowアニメーション自体は子ウィジェット側のOpacityしか動かしていないと思われるため、
		// ルート側はここで明示的に戻さないと永久に透明のままになる。
		SetRenderOpacity(1.0f);

		OnShowLogWindow();
	}

	// ログが来るたびに自動非表示タイマーをリセットする（最後のログからAutoHideDelay秒で消える）
	GetWorld()->GetTimerManager().SetTimer(AutoHideTimerHandle, this, &UWBP_LogWindow::HandleAutoHideTimeout, AutoHideDelay, false);
}

void UWBP_LogWindow::HandleAutoHideTimeout()
{
	bIsWindowVisible = false;
	OnHideLogWindow();
}
