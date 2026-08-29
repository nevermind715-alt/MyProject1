#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "MaineWindowWidget.generated.h"

class UInventoryComponent;

UCLASS()
class MYPROJECT1_API UMaineWindowWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// WBP側の変数名（Text_Shojikin）と完全一致させる
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Shojikin;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	UInventoryComponent* CachedInventoryComp;

	UFUNCTION()
	void HandleInventoryUpdated();
};
