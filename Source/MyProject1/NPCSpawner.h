// NPCSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Character.h" // キャラクターの構造体を使うためにインクルード
#include "NPCSpawner.generated.h"

UCLASS()
class MYPROJECT1_API ANPCSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANPCSpawner();

protected:
	virtual void BeginPlay() override;

public:
	// --- スポーン設定 ---

	/** スポーンさせる敵のブループリントクラス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	TSubclassOf<AMyProject1Character> EnemyClass;

	/** リスポーンまでの時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	float RespawnTime = 10.0f;

	//ここを追加：ゲーム開始から最初にスポーンするまでの時間（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	float InitialSpawnDelay = 5.0f;

	// --- 上書き用ステータス（ここで設定した値が敵にコピーされます） ---

	/** 敵のジョブデータ（戦士、モンクなど） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides")
	FDataTableRowHandle SpawnerJobRow;

	/** 敵のステータス（レベル、名前、HPなど） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides")
	FCharacterStats SpawnerStats;

	// --- AI / パトロール設定の上書き ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bOverridePatrolSettings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	float SpawnerPatrolRadius = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	float SpawnerPerceptionRadius = 1500.0f;

protected:
	// --- 内部処理 ---

	/** 実際にスポーンさせる関数 */
	void SpawnEnemy();

	/** 敵が死んだ時に呼ばれる関数 */
	UFUNCTION()
	void OnEnemyDeath(AActor* DeadActor);

	/** リスポーンタイマー */
	FTimerHandle RespawnTimerHandle;

	/** 現在スポーンしている敵の参照 */
	UPROPERTY()
	AMyProject1Character* SpawnedEnemy;

#if WITH_EDITORONLY_DATA
	/** エディタ上で場所がわかるようにするためのアイコン */
	UPROPERTY()
	class UBillboardComponent* SpriteComponent;

	/** 向いている方向を表示する矢印 */
	UPROPERTY()
	class UArrowComponent* ArrowComponent;
#endif
};