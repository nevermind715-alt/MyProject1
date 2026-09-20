#include "BlockingSpline.h"
#include "UObject/ConstructorHelpers.h"

ABlockingSpline::ABlockingSpline()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;

	// エディタで配置した瞬間から使えるよう、Engine標準のCubeを既定メッシュにしておく。
	// 厚み・高さはRebuildSplineMeshesでこのメッシュの実寸から自動スケールするため、他のメッシュに差し替えても壊れない。
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMeshAsset.Succeeded())
	{
		BlockingMesh = DefaultMeshAsset.Object;
	}
}

void ABlockingSpline::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildSplineMeshes();
}

void ABlockingSpline::RebuildSplineMeshes()
{
	for (USplineMeshComponent* Mesh : GeneratedMeshes)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}
	GeneratedMeshes.Empty();

	if (!BlockingMesh)
	{
		return;
	}

	// BlockingMeshの実寸バウンズ（Y=厚み方向, Z=高さ方向）から、指定した壁厚・壁高になるスケール比を求める。
	// SplineMeshComponentのForwardAxisは既定でX軸なので、Scale.X→メッシュのYサイズ、Scale.Y→メッシュのZサイズに対応する。
	const FBoxSphereBounds MeshBounds = BlockingMesh->GetBounds();
	const float MeshSizeY = FMath::Max(MeshBounds.BoxExtent.Y * 2.0f, KINDA_SMALL_NUMBER);
	const float MeshSizeZ = FMath::Max(MeshBounds.BoxExtent.Z * 2.0f, KINDA_SMALL_NUMBER);
	const FVector2D WallScale(WallThickness / MeshSizeY, WallHeight / MeshSizeZ);

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	const bool bClosed = Spline->IsClosedLoop();
	const int32 NumSegments = bClosed ? NumPoints : NumPoints - 1;

	for (int32 i = 0; i < NumSegments; ++i)
	{
		const int32 StartIndex = i;
		const int32 EndIndex = (i + 1) % NumPoints;

		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->SetStaticMesh(BlockingMesh);
		SplineMesh->SetCollisionProfileName(TEXT("BlockAll"));
		SplineMesh->SetVisibility(!bHideMesh);
		SplineMesh->SetupAttachment(Spline);
		SplineMesh->RegisterComponentWithWorld(GetWorld());

		const FVector StartPos = Spline->GetLocationAtSplinePoint(StartIndex, ESplineCoordinateSpace::Local);
		const FVector StartTangent = Spline->GetTangentAtSplinePoint(StartIndex, ESplineCoordinateSpace::Local);
		const FVector EndPos = Spline->GetLocationAtSplinePoint(EndIndex, ESplineCoordinateSpace::Local);
		const FVector EndTangent = Spline->GetTangentAtSplinePoint(EndIndex, ESplineCoordinateSpace::Local);

		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		SplineMesh->SetStartScale(WallScale);
		SplineMesh->SetEndScale(WallScale);

		GeneratedMeshes.Add(SplineMesh);
	}
}
