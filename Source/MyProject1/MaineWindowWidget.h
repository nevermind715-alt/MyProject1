#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "MaineWindowWidget.generated.h"

class UInventoryComponent;
class UMyProject1GameInstance;

UCLASS()
class MYPROJECT1_API UMaineWindowWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// WBP側の変数名（Text_Shojikin）と完全一致させる
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Shojikin;

	// 月経周期／妊娠中などの現在の状態名（WBP側の変数名Text_kiと一致させる。名前はGameInstanceのCyclePhaseRules等で設定）
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_ki;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	UInventoryComponent* CachedInventoryComp;

	UPROPERTY()
	UMyProject1GameInstance* CachedGameInstance;

	UFUNCTION()
	void HandleInventoryUpdated();

	UFUNCTION()
	void HandleCycleDisplayChanged();
};
