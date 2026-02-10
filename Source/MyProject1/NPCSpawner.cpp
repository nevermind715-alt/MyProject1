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

	// ゲーム開始時に最初の1体をスポーン
	SpawnEnemy();
}

void ANPCSpawner::SpawnEnemy()
{
	if (!EnemyClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 1. スポーン位置と回転を決定
	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 2. 敵をスポーン
	SpawnedEnemy = World->SpawnActor<AMyProject1Character>(EnemyClass, Location, Rotation, SpawnParams);

	if (SpawnedEnemy)
	{
		// 3. スポーナーの設定を敵に「上書きコピー」する
		// ★ここがやりたかった機能の核心です！
		SpawnedEnemy->JobRow = this->SpawnerJobRow;      // ジョブ
		SpawnedEnemy->MyStats = this->SpawnerStats;      // レベル・名前・HPなど

		// AI設定の上書き
		if (bOverridePatrolSettings)
		{
			SpawnedEnemy->PatrolRadius = this->SpawnerPatrolRadius;
			SpawnedEnemy->AISightRadius = this->SpawnerPerceptionRadius;
			// 必要なら初期位置をパトロール中心点としてセットし直す処理などを追加
		}

		// 4. 新しい設定に基づいてステータスを再計算させる
		// (レベルが変わったのでHPなどを計算しなおすため)
		SpawnedEnemy->ApplyJobData();

		// HPを全快にしておく（計算で最大HPが増えたあとに合わせるため）
		SpawnedEnemy->MyStats.HP = SpawnedEnemy->MyStats.MaxHP;

		// 5. 敵が死んだ時の通知を受け取る設定
		SpawnedEnemy->OnDeathDelegate.AddDynamic(this, &ANPCSpawner::OnEnemyDeath);
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