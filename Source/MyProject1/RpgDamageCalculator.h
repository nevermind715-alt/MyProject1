#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyProject1Types.h"
#include "RpgDamageCalculator.generated.h"

UCLASS()
class MYPROJECT1_API URpgDamageCalculator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RPG Logic")
	static FDamageResult CalculateDamage(
		const FCharacterStats& AttackerStats,
		const FCharacterStats& DefenderStats,
		float SkillDamageMultiplier = 1.0f,
		float SkillCriticalBonus = 0.0f,
		float AbilityDValue = 0.0f
	);
};