#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_ItemActionMenu.generated.h"

class UButton;
class USoundBase;

/**
 * インベントリのアイテムをクリックした時に出る「使う／捨てる／キャンセル」メニューのC++基底クラス。
 * ボタンの処理は全てここで行うため、BP側（WBP_ItemActionMenu）のグラフは不要。
 * Designerのボタン名は Btn_Use / Btn_Drop / Btn_Cancel のままでよい。
 */
UCLASS()
class MYPROJECT1_API UWBP_ItemActionMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 操作対象のアイテムID。メニューを作る側（WBP_InventorySlot）が AddToViewport の前にセットする。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn = "true"))
	FName TargetItemID;

	/** 「使う」を押した時の音（任意） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* UseSound = nullptr;

	/** 「捨てる」を押した時の音（任意） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* DropSound = nullptr;

	/** 「キャンセル」を押した時の音（任意） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* CancelSound = nullptr;

protected:
	// Widgetが生成された時に呼ばれる（BPのConstructより先）
	virtual void NativeConstruct() override;

	// InventoryComponent::OnItemActionMenuForceCloseを受け取ったら呼ばれる
	UFUNCTION()
	void HandleForceClose();

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Use;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Drop;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Cancel;

private:
	UFUNCTION()
	void HandleUseClicked();

	UFUNCTION()
	void HandleDropClicked();

	UFUNCTION()
	void HandleCancelClicked();

	/** メニューを閉じ、詳細パネルの選択も解除する（3つのボタン共通の後始末） */
	void CloseMenu(USoundBase* Sound);

	class UInventoryComponent* GetInventory() const;
};