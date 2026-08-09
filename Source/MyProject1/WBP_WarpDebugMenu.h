#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1Types.h"
#include "WBP_WarpDebugMenu.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_WarpDebugMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	// 一覧のボタンから呼ぶ。デバッグ用なのでRequiredFlagの所持チェックは無視して強制的に飛ぶ
	UFUNCTION(BlueprintCallable, Category = "Warp Debug")
	void WarpToDebugDestination(FName WarpID);

	// メニューを閉じる（マウスカーソルと入力モードを元に戻し、ビューポートから外す）
	UFUNCTION(BlueprintCallable, Category = "Warp Debug")
	void CloseDebugMenu();

protected:
	virtual void NativeConstruct() override;

	// DT_WarpDestinationsの全行を渡すので、BP側でこれを受けてScrollBox等にボタンを並べる
	UFUNCTION(BlueprintImplementableEvent, Category = "Warp Debug")
	void OnWarpListReady(const TArray<FWarpDestinationInfo>& Destinations);
};
