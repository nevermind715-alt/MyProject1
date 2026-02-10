// WBP_ShopSlot.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h" // これが必要
#include "WBP_ShopSlot.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_ShopSlot : public UUserWidget // UUserWidgetを継承
{
	GENERATED_BODY()

public:
	// スロットが保持するアイテムID（BP側でアイテム生成時にセットする）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ExposeOnSpawn))
	FName ItemID;

protected:
	// マウスが入った時の検知（cppで実装済み）
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// マウスが離れた時の検知（cppで実装済み）
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// ★追加：エディタでカーソル音を選択できるようにする
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	class USoundBase* HoverSound;

};