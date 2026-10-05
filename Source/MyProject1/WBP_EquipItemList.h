#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1Types.h"
#include "WBP_EquipItemList.generated.h"

class UButton;
class UScrollBox;
class USoundBase;
class UWBP_EquipDetail;
class UWBP_EquipItemRow;
class UWBP_EquipmentMenu;

/**
 * 装備メニューで1つの部位を選んだ時に出る「その部位に装備できる所持品の一覧」のC++基底クラス。
 * 一覧生成・装備・外す・閉じるまで全てここで行う。BP側のグラフは不要。
 */
UCLASS()
class MYPROJECT1_API UWBP_EquipItemList : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一覧に載せる部位。生成する側（WBP_EquipmentMenu）が渡す。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (ExposeOnSpawn = "true"))
	EEquipmentSlot TargetSlot = EEquipmentSlot::Torso;

	/** 戻り先の装備メニュー。生成する側が渡す。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (ExposeOnSpawn = "true"))
	UWBP_EquipmentMenu* ParentMenu = nullptr;

	/** 一覧の各行に使うウィジェットクラス。クラスデフォルトで WBP_EquipItemRow を指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TSubclassOf<UWBP_EquipItemRow> RowWidgetClass;

	/** 装備を確定した時の音（任意） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* EquipSound = nullptr;

	/** 「外す」を押した時の音（任意） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* UnequipSound = nullptr;

	/** 一覧を作り直す */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UpdateList();

	/** 行がクリックされた時に行から呼ばれる。装備して一覧を閉じる。 */
	void OnRowSelected(FName ItemID);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* ScrollBox_Items;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Unequip;

	UPROPERTY(meta = (BindWidgetOptional))
	UWBP_EquipDetail* WBP_EquipDetail;

private:
	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleUnequipClicked();

	/** この一覧を閉じ、HUDの「開いているサブメニュー」の記憶を空にする */
	void CloseList();
};