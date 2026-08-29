// MyProject1AnimInstanceBase.cpp

#include "MyProject1AnimInstanceBase.h"
#include "MyProject1Character.h"
#include "GameFramework/Character.h"
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

	// MovementComponentはAMyProject1Character専用ではなく、ACharacterなら誰でも持っている汎用機能なので、
	// AMyProject1Characterではないキャラクター（AQuestNPCBaseなど）でも独立して解決できるようにする
	ACharacter* OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
	MovementComponent = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
}
