#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "BlockingSpline.generated.h"

/**
 * スプラインに沿って見えない当たり判定メッシュを並べて配置する汎用ブロッキングアクタ。
 * 海岸線・崖・道の端など、直方体のBlocking Volumeでは合わせづらい曲線状の境界に使う。
 *
 * 使い方：
 * 1. レベルに配置し、Splineコンポーネントをエディタでポイント追加・移動して境界線の形にする
 * 2. BlockingMesh に、細長い板状でSimple Collisionを持つStaticMeshを指定する
 * 3. Splineの形を変更するたびOnConstructionで自動的に再生成される
 */
UCLASS()
class MYPROJECT1_API ABlockingSpline : public AActor
{
	GENERATED_BODY()

public:
	ABlockingSpline();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USplineComponent* Spline;

	/** 当たり判定用に使うStaticMesh（細長い板状・Simple Collision必須） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockingSpline")
	UStaticMesh* BlockingMesh;

	/** 生成したメッシュの見た目を隠すか（コリジョンには影響しない） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockingSpline")
	bool bHideMesh = true;

	/** 壁の高さ（cm）。BlockingMeshの実寸バウンズから自動でスケール計算する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockingSpline", meta = (ClampMin = "1.0"))
	float WallHeight = 300.0f;

	/** 壁の厚み（cm）。BlockingMeshの実寸バウンズから自動でスケール計算する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockingSpline", meta = (ClampMin = "1.0"))
	float WallThickness = 50.0f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient)
	TArray<USplineMeshComponent*> GeneratedMeshes;

	void RebuildSplineMeshes();
};
