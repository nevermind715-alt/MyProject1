// MyProject1AnimInstanceBase.cpp

#include "MyProject1AnimInstanceBase.h"
#include "MyProject1Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMyProject1AnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ResolveOwningCharacter();
}

void UMyProject1AnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!IsValid(Character))
	{
		ResolveOwningCharacter();
	}
}

void UMyProject1AnimInstanceBase::ResolveOwningCharacter()
{
	Character = Cast<AMyProject1Character>(TryGetPawnOwner());
	MovementComponent = Character ? Character->GetCharacterMovement() : nullptr;
}
