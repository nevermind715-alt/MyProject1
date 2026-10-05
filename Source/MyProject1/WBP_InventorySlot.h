#pragma once

#include "CoreMinimal.h"
#include "WBP_ShopSlot.h"
#include "WBP_InventorySlot.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UWBP_ItemActionMenu;

/**
 * インベントリ画面の1行（アイテム1種類）のC++基底クラス。ホバー処理は親のUWBP_ShopSlotを継承。
 * アイコン／名前／個数の表示と、クリック時のアクションメニュー表示をここで行う。
 * BP側（WBP_InventorySlot）は、親クラスをこれに変え、クラスデフォルトの ActionMenuClass に WBP_ItemActionMenu を指定するだけでよい。
 */
UCLASS()
class MYPROJECT1_API UWBP_InventorySlot : public UWBP_ShopSlot
{
	GENERATED_BODY()

public:
	/** クリックで開くアクションメニューのクラス（クラスデフォルトで WBP_ItemActionMenu を指定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSubclassOf<UWBP_ItemActionMenu> ActionMenuClass;

	/** 表示内容をセットする。InventoryScreenが生成直後に呼ぶ。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetupSlot(FName InItemID, int32 InQuantity, bool bInEquipped);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* SlotButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* QuantityText;

	/** 「装備中」の印にしたいウィジェット（任意）。置いた場合は装備中のときだけ表示される。置かない場合は名前の後ろに「【装備中】」を付ける。 */
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* EquippedMark;

private:
	UFUNCTION()
	void HandleSlotClicked();
};