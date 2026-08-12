#include "RoomMusicVolume.h"
#include "Components/BoxComponent.h"
#include "MyProject1Character.h"
#include "MusicControlComponent.h"

ARoomMusicVolume::ARoomMusicVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	RootComponent = BoxComp;
	BoxComp->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	BoxComp->SetCollisionProfileName(TEXT("Trigger"));
}

void ARoomMusicVolume::BeginPlay()
{
	Super::BeginPlay();

	BoxComp->OnComponentBeginOverlap.AddDynamic(this, &ARoomMusicVolume::OnOverlapBegin);
	BoxComp->OnComponentEndOverlap.AddDynamic(this, &ARoomMusicVolume::OnOverlapEnd);
}

void ARoomMusicVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(OtherActor);
	if (PlayerChar && PlayerChar->IsPlayerControlled() && PlayerChar->MusicComp)
	{
		PlayerChar->MusicComp->EnterRoomMusic(RoomMusic);
	}
}

void ARoomMusicVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(OtherActor);
	if (PlayerChar && PlayerChar->IsPlayerControlled() && PlayerChar->MusicComp)
	{
		PlayerChar->MusicComp->ExitRoomMusic();
	}
}
