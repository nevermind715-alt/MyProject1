#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyProject1Types.h" // さっき作ったファイルを読み込む
#include "RpgDamageCalculator.generated.h"

UCLASS()
class MYPROJECT1_API URpgDamageCalculator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 引数を「構造体」に変えます
	UFUNCTION(BlueprintCallable, Category = "RPG Logic")
	static FDamageResult CalculateDamage(const FCharacterStats& AttackerStats, const FCharacterStats& DefenderStats);
};