#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MyProject1HUD.generated.h"

class UChestComponent;

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

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsCommandMenuOpen() const;

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

protected:
	// ゲーム開始時に呼ばれる関数
	virtual void BeginPlay() override;
};