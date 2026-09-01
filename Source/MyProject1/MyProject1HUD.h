#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MyProject1HUD.generated.h"

class UChestComponent;
class UWBP_TimeSkipMenu;

UCLASS()
class MYPROJECT1_API AMyProject1HUD : public AHUD
{
	GENERATED_BODY()

public:
	// エディタ（Blueprint）側で、どのWBPを表示するか指定するためのクラス
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> PlayerStatusWidgetClass;

	// 実際に画面に生成されたウィジェットのインスタンスを保持
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* PlayerStatusWidget;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> CommandMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* CommandMenuWidget;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> InventoryMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* InventoryMenuWidget;

	// --- 施術ショップ（総合メニュー）用の変数 ---
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> TreatmentMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* TreatmentMenuWidget;

	// --- 施術ショップメニューの开闭関数 ---
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleTreatmentMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleItemShopMenu();
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> ItemShopMenuClass;
		
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* ItemShopMenuWidget;

	// --- 刺青専門店メニュー用の変数と関数 ---
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> TattooMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* TattooMenuWidget;

	UPROPERTY(BlueprintReadWrite, Category = "UI|Tattoo")
	TArray<int32> CurrentNPCShopIDs;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleTattooMenu();

	// --- 解錠屋（拘束具/呪物の解除）専門店メニュー用の変数と関数 ---
	// 刺青店（TattooMenu）と同じ構造。HUDのBPで RestraintShopMenuClass に WBP_RockBreakerShop を割り当てる。
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> RestraintShopMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* RestraintShopMenuWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleRestraintShopMenu();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsCommandMenuOpen() const;

	// コマンドメニューが開いていれば強制的に閉じる（インタラクト優先のため）。
	// 何も閉じなければ何もしない。ToggleCommandMenu()と同じ「開いている時に呼ぶと閉じる」性質を利用。
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ForceCloseCommandMenuForInteract();

	// 現在開いているすべての「サブメニュー（一覧画面など）」を記憶する汎用変数
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	class UUserWidget* ActiveSubMenuWidget;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> ChestMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* ChestMenuWidget;

	// 現在アクセスしているチェストコンポーネントを記憶
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	UChestComponent* CurrentChestComp;

	// チェストメニューの開閉関数
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleChestMenu();

	// --- クエストメニュー用の変数 ---
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> QuestMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* QuestMenuWidget;

	// --- クエストメニューの開閉関数 ---
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleQuestMenu();

	// --- 装備メニュー用の変数 ---
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> EquipmentMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* EquipmentMenuWidget;

	// --- 装備メニューの開閉関数 ---
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleEquipmentMenu();

	// ---ステータスメニュー用の変数 ---
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UUserWidget> StatusMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* StatusMenuWidget;

	// --- ステータスメニューの開閉関数 ---
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleStatusMenu();

	// --- 待機/睡眠メニュー用の変数と関数 ---
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UWBP_TimeSkipMenu> TimeSkipMenuClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UWBP_TimeSkipMenu* TimeSkipMenuWidget;

	// 待機/睡眠メニューを開く（bIsSleepModeでウィジェット側の表示を出し分ける）
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenTimeSkipMenu(bool bIsSleepMode);

	// メニューの表示・非表示を切り替える関数
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleCommandMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleInventoryMenu();

	// メニューを開く時の音
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Audio")
	USoundBase* MenuOpenSound;

	// メニューを閉じる時の音
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Audio")
	USoundBase* MenuCloseSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Audio")
	USoundBase* DialogLineSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Audio")
	USoundBase* TargetCursorSound;

	// クエスト受注時の既定音（DT_QuestData側で個別に設定されていればそちらを優先）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Audio")
	USoundBase* DefaultQuestAcceptSound;

	// クエスト完了時の既定音（DT_QuestData側で個別に設定されていればそちらを優先）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Audio")
	USoundBase* DefaultQuestCompletionSound;

	// クエスト目的達成時の既定音（DT_QuestData側で個別に設定されていればそちらを優先）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Audio")
	USoundBase* DefaultQuestObjectiveClearedSound;

protected:
	// ゲーム開始時に呼ばれる関数
	virtual void BeginPlay() override;
};