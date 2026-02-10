#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UAnimNotify_Footstep.generated.h"

UCLASS()
class MYPROJECT1_API UAnimNotify_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	// Received_Notify ではなく Notify を使用し、末尾の const も外します
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};