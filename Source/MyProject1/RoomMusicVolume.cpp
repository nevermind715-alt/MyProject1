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

	// プレイヤーキャラクター専用のObject Type(ECC_GameTraceChannel1 = "Player")だけをOverlap検知し、
	// それ以外の全チャンネル(Pawn=NPC含む)は無視することで、他のアクタを物理的にブロックしないようにする
	BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
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
		PlayerChar->MusicComp->EnterRoomMusic(RoomMusic, Volume);
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
