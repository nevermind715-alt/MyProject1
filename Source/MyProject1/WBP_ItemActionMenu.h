#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_ItemActionMenu.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_ItemActionMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	// Widgetが生成された時に呼ばれる（BPのConstructより先）
	virtual void NativeConstruct() override;

	// InventoryComponent::OnItemActionMenuForceCloseを受け取ったら呼ばれる
	UFUNCTION()
	void HandleForceClose();
};
