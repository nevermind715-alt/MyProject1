#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Engine/DataTable.h" // データテーブルを使うために追加
#include "MyProject1Types.h"  // FItemData を使うために追加
#include "ItemSpawner.generated.h"

UCLASS()
class MYPROJECT1_API AItemSpawner : public AActor
{
	GENERATED_BODY()

public:
	AItemSpawner();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* CollisionComp;

	// --- スポナーの設定 ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	FName ItemIDToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	int32 SpawnAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	bool bIsOneTimeOnly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (EditCondition = "!bIsOneTimeOnly"))
	float RespawnTime = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	class USoundBase* PickupSound;

	// アイテムの名前を取得するために、データテーブルをセットする枠を追加
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	UDataTable* ItemDataTable;

	// --- 判定イベント ---
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 離れた時のイベントを追加
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// --- アクション ---
	// プレイヤーが「Eキー」を押した時に呼び出す関数
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void Interact(class AMyProject1Character* InteractingPlayer);

	// --- UI連携 ---
	// BP側に「UIを表示/非表示にして！」とお願いするイベント
	// 引数でアイテム名と個数を渡せるので、BP側で文字の書き換えが簡単にできます
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner|UI")
	void OnToggleInteractUI(bool bShow, const FString& ItemName, int32 Amount);

private:
	FTimerHandle RespawnTimerHandle;
	void RespawnItem();

	bool bIsActive = true;
};