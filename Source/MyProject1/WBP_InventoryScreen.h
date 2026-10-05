#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1Types.h"
#include "WBP_InventoryScreen.generated.h"

class UButton;
class UScrollBox;
class UWBP_InventorySlot;

/**
 * インベントリ画面（持ち物一覧）のC++基底クラス。
 * 開閉はAMyProject1HUD::ToggleInventoryMenuが管理する。
 * タブ切替・カテゴリ絞り込み・一覧生成・インベントリ更新時の再描画・閉じるボタンまで全てここで行うため、
 * BP側（WBP_InventoryScreen）のグラフは不要（Designerの見た目だけ使う）。
 */
UCLASS()
class MYPROJECT1_API UWBP_InventoryScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一覧の各行に使うウィジェットクラス。クラスデフォルトで WBP_InventorySlot を指定する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSubclassOf<UWBP_InventorySlot> InventorySlotClass;

	/** 今開いているタブ（カテゴリ）。初期表示は武器。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	EItemType ActiveTab = EItemType::Weapon;

	/** タブを切り替えて一覧を作り直す */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SelectTab(EItemType NewTab);

	/** 現在のタブで一覧を作り直す。インベントリが変わるたびに自動で呼ばれる。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshList();

	/** 閉じる要求。HUDのToggleInventoryMenuへ中継する。 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventoryScreen();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- BP側の変数名と完全一致させる（全てBindWidgetOptionalなので未配置でも安全） ---
	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* ItemScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Wepon;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Potion;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Armor;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Food;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_material;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_etc;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_KeyItem;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Close;

private:
	UFUNCTION() void HandleWeaponClicked();
	UFUNCTION() void HandlePotionClicked();
	UFUNCTION() void HandleArmorClicked();
	UFUNCTION() void HandleFoodClicked();
	UFUNCTION() void HandleMaterialClicked();
	UFUNCTION() void HandleEtcClicked();
	UFUNCTION() void HandleKeyItemClicked();
	UFUNCTION() void HandleCloseClicked();

	/** InventoryComponent::OnInventoryUpdated を受けて再描画する */
	UFUNCTION()
	void HandleInventoryUpdated();

	class UInventoryComponent* GetInventory() const;
};