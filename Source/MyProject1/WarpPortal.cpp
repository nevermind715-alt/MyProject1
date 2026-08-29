#include "WarpPortal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "MyProject1GameInstance.h"

#pragma execution_character_set("utf-8")

AWarpPortal::AWarpPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	CollisionBox->InitBoxExtent(FVector(100.f, 100.f, 100.f));

	CollisionBox->SetCollisionProfileName(TEXT("Trigger"));

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWarpPortal::OnOverlapBegin);
}

void AWarpPortal::BeginPlay()
{
	Super::BeginPlay();
}

void AWarpPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->ActorHasTag(TEXT("Player"))) return;

	if (ACharacter* PlayerChar = Cast<ACharacter>(OtherActor))
	{
		if (PortalType == EWarpPortalType::TouchToWarp)
		{
			ExecuteWarp(PlayerChar);
		}
	}
}

void AWarpPortal::InteractWithPortal(ACharacter* Interactor)
{
	if (PortalType == EWarpPortalType::InteractToWarp)
	{
		ExecuteWarp(Interactor);
	}
}

void AWarpPortal::ExecuteWarp(ACharacter* TargetCharacter)
{
	if (!TargetCharacter || TargetWarpID.IsNone()) return;

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInst->RequestWarp(TargetWarpID, TargetCharacter);
	}
}
