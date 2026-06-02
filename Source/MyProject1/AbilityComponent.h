#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // データテーブルの構造体を使うために追加
#include "AbilityComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- データテーブル ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	class UDataTable* AbilityDataTable;

	// --- アビリティの習得と管理 ---
	// キャラクターが現在覚えているアビリティのIDリスト
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Ability")
	TArray<FName> LearnedAbilities;

	// リキャスト（クールダウン）中のアビリティリスト（ID -> 残り秒数）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability")
	TMap<FName, float> RecastTimers;

	// 新しいアビリティを覚える関数
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void LearnAbility(FName AbilityID);

	// --- 詠唱（キャスト）の管理 ---
	// 現在詠唱中かどうか
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Casting")
	bool bIsCasting;

	// 現在詠唱しているアビリティのID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Casting")
	FName CurrentCastingAbilityID;

	// 魔法の発動対象（ターゲット）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Casting")
	AActor* CurrentTarget;

	// アビリティを使おうと試みる関数（TP/MP不足やリキャスト中なら弾く）
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool TryCastAbility(FName AbilityID, AActor* TargetActor);

	// 詠唱を中断する関数（ダメージを受けた時や動いた時などに呼ぶ）
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void CancelCasting();

private:
	// 詠唱タイマーの管理用
	FTimerHandle CastTimerHandle;

	// 詠唱完了後に実際に効果を発動する内部関数
	void ExecuteAbility();
};