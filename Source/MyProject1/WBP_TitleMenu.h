#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "WBP_TitleMenu.generated.h"

class UWBP_SaveSlot;
class USoundBase;
class UAudioComponent;
class UWBP_FadeNarration;

/**
 * タイトル画面（ニューゲーム／つづきから／終了）のC++基底クラス。
 * 一覧生成・ロード実行・画面切り替えまで全てここで行うため、
 * BP側（WBP_TitleMenu）はDesignerでの見た目作りと、下記のBindWidget名合わせ・
 * SaveSlotWidgetClass・NewGameLevelの指定だけでよい。グラフに実装すべきロジックは無い。
 * 一覧の行はセーブ画面と同じUWBP_SaveSlot（のBP）を流用する（ロード専用表示にして使う）。
 */
UCLASS()
class MYPROJECT1_API UWBP_TitleMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 一覧の各行に使うウィジェットクラス。BP側のクラスデフォルトでWBP_SaveSlotを指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	TSubclassOf<UWBP_SaveSlot> SaveSlotWidgetClass;

	/** 「ニューゲーム」で開くレベル。BP側のクラスデフォルトで指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
	TSoftObjectPtr<UWorld> NewGameLevel;

	/** タイトル画面で流すBGM。ループさせたい場合は、Sound Wave側のLoopingを有効にしておく。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Sound")
	TObjectPtr<USoundBase> TitleBGM;

	/** 「ニューゲーム」でレベルを開く前に、黒い画面で1ページずつ表示するあらすじ。空なら演出を挟まず即座にレベルを開く。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Intro", meta = (MultiLine = true))
	TArray<FText> IntroPages;

	/** あらすじの表示に使うウィジェットクラス（暗転セリフと同じWBP_FadeNarration）。未指定ならあらすじは出さない。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title|Intro")
	TSubclassOf<UWBP_FadeNarration> IntroNarrationClass;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// --- WBP側の変数名と完全一致させる（すべてBindWidgetOptionalなので、未配置でも安全に動く） ---

	/** タイトルのメインボタン群を入れた箱（「つづきから」一覧を開いている間は隠す） */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Panel_Main;

	/** 「つづきから」のセーブ一覧全体（初期状態はC++側で隠す） */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Panel_Continue;

	/** 一覧の行を入れる箱（ScrollBox/VerticalBoxどちらでも良い） */
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* Scroll_Slots;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_NewGame;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Continue;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Quit;

	/** 一覧からメインへ戻るボタン */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ContinueBack;

	/** あらすじ表示中に画面全体を覆う黒い背景（Designerで全画面の黒いImage/Borderを一番手前に置く。初期状態はC++側で隠す） */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* Img_IntroBackground;

private:
	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleContinueBackClicked();

	UFUNCTION()
	void HandleLoadRequested(const FString& SlotName);

	// 再生中のタイトルBGM。NativeDestructで止めるため保持する。
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> TitleBGMComponent;

	// あらすじ表示中のウィジェットとページ位置
	UPROPERTY(Transient)
	TObjectPtr<UWBP_FadeNarration> IntroNarrationWidget;

	bool bIntroActive = false;
	int32 IntroPageIndex = 0;

	void BeginIntro();
	void AdvanceIntro();
	void StartNewGame();

	/** ゲーム開始（レベル遷移）直前に、タイトル用のUI専用入力モード・カーソル表示をゲーム操作用へ戻す */
	void RestoreGameInput();

	void BuildSlotList();
	void ShowMainPanel();
};
