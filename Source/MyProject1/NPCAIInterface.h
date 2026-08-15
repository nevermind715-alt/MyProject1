#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NPCAIInterface.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable, BlueprintType)
class UNPCAIInterface : public UInterface
{
	GENERATED_BODY()
};

// AMyAIController（徘徊・索敵AI）が操作対象Pawnに要求する最小限のAPI。
// AMyProject1Character（戦闘NPC）とAQuestNPCBase（会話NPC）の両方がこれを実装することで、
// AIコントローラー側はCast<AMyProject1Character>で決め打ちせずに同じAIロジックを流用できる。
// 戦闘機能を持たない実装は IsActiveEnemy() を常にfalse、索敵系の値を0にすればよい
// （AMyAIController::OnTargetDetectedは非アクティブ敵かつ未ターゲット状態なら何もしないため）。
class MYPROJECT1_API INPCAIInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual bool CanPatrol() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetPatrolWalkSpeed() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetChaseRunSpeed() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetPatrolRadius() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetPatrolWaitMin() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetPatrolWaitMax() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetAISightRadius() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetAILoseSightRadius() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetAIVisionAngle() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual bool IsAIHearingEnabled() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual float GetAIHearingRange() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual bool IsActiveEnemy() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual bool GetNeverLoseSight() const = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual void SetCurrentTarget(AActor* NewTarget) = 0;

	UFUNCTION(BlueprintCallable, Category = "AI")
	virtual bool IsDead() const = 0;
};
