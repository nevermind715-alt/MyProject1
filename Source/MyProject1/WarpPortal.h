#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Types.h" // EWarpPortalTypeなどを使うため
#include "WarpPortal.generated.h"

UCLASS()
class MYPROJECT1_API AWarpPortal : public AActor
{
	GENERATED_BODY()

public:
	AWarpPortal();

protected:
	virtual void BeginPlay() override;

public:
	// --- コンポーネント ---
	/** 当たり判定（触れたことを検知するボックス） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* CollisionBox;

	// --- ワープ設定（エディタで自由に設定可能） ---
	/** 飛ぶ先のWarpID（DT_WarpDestinations に登録した行名） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	FName TargetWarpID;

	/** ワープの起動方法（触れるか、調べるか） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	EWarpPortalType PortalType = EWarpPortalType::TouchToWarp;

	// 確認時に呼び出す会話データ（DT_Dialogs）の行名
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	FName ConfirmDialogID;

	// --- 処理関数 ---
	/** プレイヤーが触れた時に呼ばれる関数 */
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** プレイヤーがEキーなどで「調べた」時に呼ばれる関数（BPから呼ぶ） */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void InteractWithPortal(class ACharacter* Interactor);

private:
	/** 実際のワープ処理（GameInstanceを呼ぶ） */
	void ExecuteWarp(class ACharacter* TargetCharacter);
};