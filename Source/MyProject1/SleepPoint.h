#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SleepPoint.generated.h"

/**
 * ベッドや布団に置く、睡眠用のインタラクトポイント。
 * QuestItemPointと同じ「Tagsに"NPC"を付けてInteractRange判定に乗せる」方式で、
 * インタラクトされたら既存のTryOpenTimeSkipMenu(true)を呼んで睡眠メニュー(WBP_TimeSkipMenu)を開くだけの窓口。
 * 時間を進める処理・暗転・UIは全て既存の仕組み（WBP_TimeSkipMenu/GameInstance）に任せる。
 */
UCLASS()
class MYPROJECT1_API ASleepPoint : public AActor
{
	GENERATED_BODY()

public:
	ASleepPoint();

	// --- 見た目 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* Mesh;

	/** プレイヤーがインタラクトした時に呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "Sleep")
	void TryInteract(class AMyProject1Character* Interactor);
};
