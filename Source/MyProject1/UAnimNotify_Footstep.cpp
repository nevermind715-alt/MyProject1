#include "UAnimNotify_Footstep.h" 
#include "MyProject1Character.h"

// 関数名を Notify に変更し、const を外します
void UAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference); // Super も Notify に変更

	if (!MeshComp) return;

	AMyProject1Character* Character = Cast<AMyProject1Character>(MeshComp->GetOwner());
	if (Character)
	{
		Character->PlayFootstepSound();
	}
}