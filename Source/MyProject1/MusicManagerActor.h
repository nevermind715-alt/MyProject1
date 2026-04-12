#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MusicManagerActor.generated.h"

UCLASS()
class MYPROJECT1_API AMusicManagerActor : public AActor
{
	GENERATED_BODY()

public:
	AMusicManagerActor();

	// このレベルで流すフィールド曲
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Music")
	USoundBase* FieldMusic;

	// このレベルで流す戦闘曲
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Music")
	USoundBase* BattleMusic;
};