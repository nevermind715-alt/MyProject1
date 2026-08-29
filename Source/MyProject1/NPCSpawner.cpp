// NPCSpawner.cpp

#include "NPCSpawner.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/KismetMathLibrary.h"

ANPCSpawner::ANPCSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	RootComponent = SpriteComponent;

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	ArrowComponent->SetupAttachment(RootComponent);
#endif
}

void ANPCSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (InitialSpawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ANPCSpawner::SpawnEnemy, InitialSpawnDelay, false);
	}
	else
	{
		SpawnEnemy();
	}
}

void ANPCSpawner::SpawnEnemy()
{
	if (!EnemyClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();
	FTransform SpawnTransform(Rotation, Location);

	SpawnedEnemy = World->SpawnActorDeferred<AMyProject1Character>(
		EnemyClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (SpawnedEnemy)
	{

		FDataTableRowHandle FinalJobRow = this->SpawnerJobRow;
		FString FinalNPCName = this->SpawnerNPCName;
		FVector FinalScale = FVector(1.0f, 1.0f, 1.0f);

		if (bEnableRareSpawn)
		{
			if (FMath::RandRange(0.0f, 100.0f) <= RareSpawnChance)
			{
				FinalJobRow = this->RareJobRow;
				FinalNPCName = this->RareNPCName;
				FinalScale = this->RareScale;
			}
		}

		SpawnedEnemy->JobRow = FinalJobRow;
		SpawnedEnemy->MyStats.NPCName = FinalNPCName;
		SpawnedEnemy->MyStats.Level = this->SpawnerLevel;
		SpawnedEnemy->SetActorScale3D(FinalScale);

		if (bOverridePatrolSettings)
		{
			SpawnedEnemy->PatrolRadius = this->SpawnerPatrolRadius;
			SpawnedEnemy->bCanPatrol = this->bSpawnerCanPatrol;
			SpawnedEnemy->bIsActiveEnemy = this->bSpawnerIsActiveEnemy;
			SpawnedEnemy->bCanLink = this->bSpawnerCanLink;

			SpawnedEnemy->AISightRadius = this->SpawnerSightRadius;
			SpawnedEnemy->AILoseSightRadius = this->SpawnerLoseSightRadius;
			SpawnedEnemy->AIVisionAngle = this->SpawnerVisionAngle;
			SpawnedEnemy->bAIEnableHearing = this->bSpawnerEnableHearing;
			SpawnedEnemy->AIHearingRange = this->SpawnerHearingRange;
		}

		SpawnedEnemy->OnDeathDelegate.AddDynamic(this, &ANPCSpawner::OnEnemyDeath);

		// ---------------------------------------------------------
		// ---------------------------------------------------------
		SpawnedEnemy->FinishSpawning(SpawnTransform);

		// ---------------------------------------------------------
		// ---------------------------------------------------------
		SpawnedEnemy->ApplyJobData();

		
		
		if (bOverrideBaseLoot)
		{
			SpawnedEnemy->PersonalLootTable = this->SpawnerLootTable;
		}
		else
		{
			SpawnedEnemy->PersonalLootTable.Append(this->SpawnerLootTable);
		}

		// =========================================================
		// =========================================================
		SpawnedEnemy->MyStats.MaxHP += SpawnerStatBonus.MaxHP;
		SpawnedEnemy->MyStats.BaseAttackPower += SpawnerStatBonus.AttackPower;
		SpawnedEnemy->MyStats.BaseDefensePower += SpawnerStatBonus.DefensePower;
		SpawnedEnemy->MyStats.STR += SpawnerStatBonus.STR;
		SpawnedEnemy->MyStats.VIT += SpawnerStatBonus.VIT;
		SpawnedEnemy->MyStats.DEX += SpawnerStatBonus.DEX;
		SpawnedEnemy->MyStats.AGI += SpawnerStatBonus.AGI;
		SpawnedEnemy->MyStats.Accuracy += SpawnerStatBonus.Accuracy;
		SpawnedEnemy->MyStats.Evasion += SpawnerStatBonus.Evasion;

		SpawnedEnemy->RecalculateFatigueAdjustedCombatStats();

		SpawnedEnemy->MyStats.HP = SpawnedEnemy->MyStats.MaxHP;
	}
} 

void ANPCSpawner::OnEnemyDeath(AActor* DeadActor)
{
	if (DeadActor == SpawnedEnemy)
	{
		SpawnedEnemy = nullptr;

		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ANPCSpawner::SpawnEnemy, RespawnTime, false);
	}
}
