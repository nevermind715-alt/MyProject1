#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "MyProject1Types.h" // ★ワープ先の構造体を使うために追加
#include "MyProject1GameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarpFadeOutRequested);

UCLASS()
class MYPROJECT1_API UMyProject1GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// --- ワープ設定 ---

	UPROPERTY(BlueprintAssignable, Category = "Warp")
	FOnWarpFadeOutRequested OnWarpFadeOutRequested;

	/** ワープ先名簿（データテーブル）をセットする場所 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	UDataTable* WarpDataTable;

	// --- ワープの記憶領域 ---

	/** マップ移動を伴うワープの待機中（ロード中）か？ */
	UPROPERTY(BlueprintReadOnly, Category = "Warp")
	bool bHasPendingWarp = false;

	/** 移動先の座標と向きの記憶 */
	UPROPERTY(BlueprintReadOnly, Category = "Warp")
	FTransform PendingWarpTransform;

	// --- ワープ実行関数 ---

	/** ワープを要求する（同じマップなら即移動、別マップならロードを挟む） */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void RequestWarp(FName WarpID, class ACharacter* PlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Warp")
	void ExecuteWarpProcess();

	/** マップのロード完了後に呼ばれ、記憶した座標にプレイヤーを動かす */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void ApplyPendingWarp(class ACharacter* PlayerCharacter);

private:
	// ★追加：暗転が終わるまで待機している「ワープID」と「プレイヤー」の記憶
	FName ReservedWarpID;
	TWeakObjectPtr<class ACharacter> ReservedPlayer;
};