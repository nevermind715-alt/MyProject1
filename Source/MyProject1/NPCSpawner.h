// NPCSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Character.h" // キャラクターの構造体を使うためにインクルード
#include "NPCSpawner.generated.h"

USTRUCT(BlueprintType)
struct FStatBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AttackPower = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DefensePower = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 STR = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 VIT = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DEX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AGI = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Accuracy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Evasion = 0;
		
};

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

	/** スポーンする敵のレベル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Base Stats")
	int32 SpawnerLevel = 1;

	//FName から FString に変更しました！
	/** スポーンする敵の名前 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Base Stats")
	FString SpawnerNPCName = TEXT("Goblin");

	//ステータス補正値（ここで設定した数値がプラス・マイナスされます）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Stat Bonus")
	FStatBonus SpawnerStatBonus;

	// --- AI / パトロール設定の上書き ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bOverridePatrolSettings = true;

	// このスポーナーから出る敵はパトロールするか？
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bSpawnerCanPatrol = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	float SpawnerPatrolRadius = 1000.0f;

	// --- 追加：AIの知覚（目と耳）設定 ---

	/** 敵を見つける距離（元の SpawnerPerceptionRadius） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerSightRadius = 1500.0f;

	/** 敵を見失う距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerLoseSightRadius = 2000.0f;

	/** 視野角（90で前方180度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerVisionAngle = 90.0f;

	/** 聴覚を有効にするか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	bool bSpawnerEnableHearing = true;

	/** 聴覚の範囲 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerHearingRange = 3000.0f;

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