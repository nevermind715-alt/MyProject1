#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1Types.h"
#include "WBP_EquipmentMenu.generated.h"

class UButton;
class UTextBlock;
class UWBP_EquipItemList;

/**
 * 装備メニュー（部位ごとのボタンと、今着ている装備名の一覧）のC++基底クラス。
 * 開閉はAMyProject1HUD::ToggleEquipmentMenuが管理する。
 * 部位ボタンを押すと、その部位に装備できる所持品の一覧（WBP_EquipItemList）を開く。
 * BP側のグラフは不要（Designerの見た目だけ使う）。
 */
UCLASS()
class MYPROJECT1_API UWBP_EquipmentMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 部位ボタンを押した時に開く装備一覧のクラス。クラスデフォルトで WBP_EquipItemList を指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TSubclassOf<UWBP_EquipItemList> EquipItemListClass;

	/** 全部位の「現在の装備名」表示を更新する（未装備は「---」） */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UpdateEquipmentDisplay();

	/** 指定部位の装備一覧を開く（既に開いていれば閉じてから開き直す） */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void OpenEquipList(EEquipmentSlot TargetEquipSlot);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	// --- BP側の変数名と完全一致させる（全てBindWidgetOptional） ---
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Wepon;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Wepon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Head;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Head;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Neck;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Neck;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Torso;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Torso;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Waist;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Waist;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Wrist;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Wrist;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Hand;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Hand;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Legs;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Legs;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Ankle;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Ankle;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Foot;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Foot;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_InnerU;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_InnerUpper;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_InnerL;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_InnerLower;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Ather1;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Ather1;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Ather2;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Ather2;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Ather3;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Ather3;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Ather4;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Ather4;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Ather5;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Txt_Ather5;


private:
	UFUNCTION() void HandleWeponClicked();
	UFUNCTION() void HandleHeadClicked();
	UFUNCTION() void HandleNeckClicked();
	UFUNCTION() void HandleTorsoClicked();
	UFUNCTION() void HandleWaistClicked();
	UFUNCTION() void HandleWristClicked();
	UFUNCTION() void HandleHandClicked();
	UFUNCTION() void HandleLegsClicked();
	UFUNCTION() void HandleAnkleClicked();
	UFUNCTION() void HandleFootClicked();
	UFUNCTION() void HandleInnerUClicked();
	UFUNCTION() void HandleInnerLClicked();
	UFUNCTION() void HandleAther1Clicked();
	UFUNCTION() void HandleAther2Clicked();
	UFUNCTION() void HandleAther3Clicked();
	UFUNCTION() void HandleAther4Clicked();
	UFUNCTION() void HandleAther5Clicked();

	UPROPERTY()
	UWBP_EquipItemList* CurrentListWidget = nullptr;

	UTextBlock* GetSlotText(EEquipmentSlot EquipSlot) const;
};