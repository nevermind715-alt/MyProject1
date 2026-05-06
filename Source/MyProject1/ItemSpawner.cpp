#include "ItemSpawner.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

#pragma execution_character_set("utf-8")

AItemSpawner::AItemSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = MeshComp;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->SetupAttachment(RootComponent);
	CollisionComp->SetSphereRadius(150.0f); // UIが出る距離（少し広めに設定）
	CollisionComp->SetCollisionProfileName(TEXT("Trigger"));
}

void AItemSpawner::BeginPlay()
{
	Super::BeginPlay();

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AItemSpawner::OnOverlapBegin);

	// 離れた時のイベントも登録
	CollisionComp->OnComponentEndOverlap.AddDynamic(this, &AItemSpawner::OnOverlapEnd);
}

void AItemSpawner::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsActive || ItemIDToSpawn.IsNone()) return;

	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(OtherActor);
	if (PlayerChar && PlayerChar->IsPlayerControlled())
	{
		// アイテム名をデータテーブルから探す
		FString DisplayName = TEXT("不明なアイテム");
		if (ItemDataTable)
		{
			FItemData* ItemInfo = ItemDataTable->FindRow<FItemData>(ItemIDToSpawn, TEXT("SpawnerContext"));
			if (ItemInfo)
			{
				DisplayName = ItemInfo->Name;
			}
		}

		// BPで作ったUIを表示するイベントを呼ぶ（True）
		OnToggleInteractUI(true, DisplayName, SpawnAmount);
	}
}

void AItemSpawner::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(OtherActor);
	if (PlayerChar && PlayerChar->IsPlayerControlled())
	{
		// 範囲から出たらUIを消す（False）
		OnToggleInteractUI(false, TEXT(""), 0);
	}
}

void AItemSpawner::Interact(AMyProject1Character* InteractingPlayer)
{
	if (!bIsActive || !InteractingPlayer) return;

	if (InteractingPlayer->InventoryComp)
	{
		bool bSuccess = InteractingPlayer->InventoryComp->AddItem(ItemIDToSpawn, SpawnAmount);

		if (bSuccess)
		{
			if (PickupSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
			}

			// 取得したのでUIを消す
			OnToggleInteractUI(false, TEXT(""), 0);

			MeshComp->SetVisibility(false);
			bIsActive = false;

			if (bIsOneTimeOnly)
			{
				Destroy();
			}
			else
			{
				GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AItemSpawner::RespawnItem, RespawnTime, false);
			}
		}
		else
		{
			InteractingPlayer->OnReceiveLogMessage(TEXT("カバンがいっぱいで拾えない。"), ELogMessageType::System);
		}
	}
}

void AItemSpawner::RespawnItem()
{
	MeshComp->SetVisibility(true);
	bIsActive = true;
}