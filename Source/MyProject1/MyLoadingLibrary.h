#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyLoadingLibrary.generated.h"

UCLASS()
class MYPROJECT1_API UMyLoadingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ブループリントからいつでも呼べる「世界は読み込み完了したか？」ノード
	UFUNCTION(BlueprintPure, Category = "Loading", meta = (WorldContext = "WorldContextObject"))
	static bool IsWorldFullyLoaded(const UObject* WorldContextObject);
};