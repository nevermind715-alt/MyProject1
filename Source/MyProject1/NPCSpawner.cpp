// NPCSpawner.cpp

#include "NPCSpawner.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/KismetMathLibrary.h"

ANPCSpawner::ANPCSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// エディタでの見た目設定（ビルボードと矢印）
#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	RootComponent = SpriteComponent;

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	ArrowComponent->SetupAttachment(RootComponent);
#endif

	// デフォルトの設定
	SpawnerStats.Level = 1;
	SpawnerStats.NPCName = TEXT("Goblin");
}

void ANPCSpawner::BeginPlay()
{
	Super::BeginPlay();

	// 即座にスポーンするのではなく、タイマーで待機する
	if (InitialSpawnDelay > 0.0f)
	{
		// InitialSpawnDelay 秒後に SpawnEnemy を実行する
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ANPCSpawner::SpawnEnemy, InitialSpawnDelay, false);
	}
	else
	{
		// 待機時間が0なら、今まで通りすぐにスポーンさせる
		SpawnEnemy();
	}
}

void ANPCSpawner::SpawnEnemy()
{
	if (!EnemyClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 1. スポーン位置と回転を決定し、Transformにまとめる
	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();
	FTransform SpawnTransform(Rotation, Location);

	// 2. スポーン処理を「保留（Deferred）」状態で開始する
	SpawnedEnemy = World->SpawnActorDeferred<AMyProject1Character>(
		EnemyClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (SpawnedEnemy)
	{
		// 3. AIが憑依する前、かつ BeginPlay の「前」に設定を上書きする
		SpawnedEnemy->JobRow = this->SpawnerJobRow;      // ジョブ
		SpawnedEnemy->MyStats = this->SpawnerStats;      // レベル・名前など

		// AI設定の上書き
		if (bOverridePatrolSettings)
		{
			SpawnedEnemy->PatrolRadius = this->SpawnerPatrolRadius;
			SpawnedEnemy->AISightRadius = this->SpawnerPerceptionRadius;
			SpawnedEnemy->bCanPatrol = this->bSpawnerCanPatrol;
		}

		// 敵が死んだ時の通知を受け取る設定
		SpawnedEnemy->OnDeathDelegate.AddDynamic(this, &ANPCSpawner::OnEnemyDeath);

		// ---------------------------------------------------------
		// 4. ★保留していたスポーン処理をここで完了させる！
		// ！！！この FinishSpawning の中で敵の BeginPlay が実行され、
		//     ブループリント側で HP_Bar などのUIが作成・準備されます ！！！
		// ---------------------------------------------------------
		SpawnedEnemy->FinishSpawning(SpawnTransform);

		// ---------------------------------------------------------
		// 5. ★順番を後ろに変更：UIの準備が完了した「後」にステータスを計算する
		// ---------------------------------------------------------
		SpawnedEnemy->ApplyJobData();

		// HPを全快にしておく
		SpawnedEnemy->MyStats.HP = SpawnedEnemy->MyStats.MaxHP;

		// ※ブループリント側で「HPが変化した時にUIを更新する処理」があれば、
		// UIが準備済みのこのタイミングならエラーなく安全に動作します。
	}
}
void ANPCSpawner::OnEnemyDeath(AActor* DeadActor)
{
	// 念のため、死んだのが管理している敵かチェック
	if (DeadActor == SpawnedEnemy)
	{
		SpawnedEnemy = nullptr;

		// リスポーンタイマーをセット
		// RespawnTime秒後に SpawnEnemy を実行する
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ANPCSpawner::SpawnEnemy, RespawnTime, false);
	}
}