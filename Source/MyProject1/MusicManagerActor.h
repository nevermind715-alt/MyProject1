// MusicManagerActor.h の中身を以下のように変更
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Music")
	TSoftObjectPtr<USoundBase> FieldMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Music")
	TSoftObjectPtr<USoundBase> BattleMusic;
};