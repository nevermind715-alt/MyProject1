#include "AmbientWalkerNPC.h"
#include "MyProject1GameInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

/** 近づいたか・Bに着いたか・時間帯の内外の判定間隔（秒）。出現/消失の遅れは最大でこの秒数 */
static constexpr float AmbientWalkerCheckInterval = 0.5f;

/** 地面トレースの間隔（秒）と、地面の高さへの追従の速さ（FInterpToのInterpSpeed） */
static constexpr float AmbientWalkerGroundTraceInterval = 0.1f;
static constexpr float AmbientWalkerGroundInterpSpeed = 10.0f;

AAmbientWalkerNPC::AAmbientWalkerNPC()
{
	// Tickは「歩いていて、かつプレイヤーが近い人が居る間」だけ有効にする（それ以外はタイマーの判定だけ）
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Route = CreateDefaultSubobject<USplineComponent>(TEXT("Route"));
	Route->SetupAttachment(Root);
	// 初期状態で2点目を前方10mに置く（既定の100cmでは短すぎて配置時に分かりにくい）
	Route->SetLocationAtSplinePoint(1, FVector(1000.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, true);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	// 遠い/小さく映るメッシュのアニメ更新を間引く（URO）。描画されていない間はポーズ更新自体を行わない
	Mesh->bEnableUpdateRateOptimizations = true;
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
}

void AAmbientWalkerNPC::BeginPlay()
{
	Super::BeginPlay();

	// 1体目はMesh、2体目以降はMeshの複製
	const int32 Count = FMath::Max(WalkerCount, 1);
	Walkers.SetNum(Count);
	Walkers[0].MeshComp = Mesh;
	for (int32 Index = 1; Index < Count; ++Index)
	{
		USkeletalMeshComponent* Clone = CreateCloneMesh();
		CloneMeshes.Add(Clone);
		Walkers[Index].MeshComp = Clone;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		USkeletalMeshComponent* WalkerMesh = Walkers[Index].MeshComp;
		if (WalkAnimation)
		{
			WalkerMesh->PlayAnimation(WalkAnimation, true);
			WalkerMesh->SetPlayRate(WalkAnimationPlayRate);
			if (Index > 0)
			{
				// 全員が同じ足の動きで揃って歩かないよう、2体目以降はアニメの再生位置をずらす
				WalkerMesh->SetPosition(FMath::FRandRange(0.0f, WalkAnimation->GetPlayLength()), false);
			}
		}

		// 開始時は消しておき、タイマーの判定で歩行中かつ近い時だけ表示する
		WalkerMesh->SetVisibility(false);
	}

	SetActorTickEnabled(false);
	ActiveWalkerCount = 0;

	GetWorldTimerManager().SetTimer(ActivationTimerHandle, this, &AAmbientWalkerNPC::UpdateActivation, AmbientWalkerCheckInterval, true);
}

USkeletalMeshComponent* AAmbientWalkerNPC::CreateCloneMesh()
{
	USkeletalMeshComponent* Clone = NewObject<USkeletalMeshComponent>(this);
	Clone->SetupAttachment(GetRootComponent());
	Clone->SetSkeletalMeshAsset(Mesh->GetSkeletalMeshAsset());
	Clone->SetCollisionProfileName(TEXT("NoCollision"));
	Clone->bEnableUpdateRateOptimizations = Mesh->bEnableUpdateRateOptimizations;
	Clone->VisibilityBasedAnimTickOption = Mesh->VisibilityBasedAnimTickOption;
	Clone->SetRelativeScale3D(Mesh->GetRelativeScale3D());
	Clone->SetCastShadow(Mesh->CastShadow);

	// レベル上のインスタンスでマテリアルを差し替えていても、同じ見た目にする
	const int32 NumMaterials = Mesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
	{
		Clone->SetMaterial(MaterialIndex, Mesh->GetMaterial(MaterialIndex));
	}

	Clone->RegisterComponent();
	return Clone;
}

void AAmbientWalkerNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int32 Index = 0; Index < Walkers.Num(); ++Index)
	{
		if (!Walkers[Index].bActive)
		{
			continue;
		}

		float Distance = 0.0f;
		if (!EvaluateWalk(Index, Distance))
		{
			// Bに着いた：消える（全員が止まればTickも止まる。再開はタイマーの判定）
			SetWalkerActive(Index, false);
			continue;
		}

		PlaceMesh(Index, Distance, DeltaSeconds);
	}
}

bool AAmbientWalkerNPC::EvaluateWalk(int32 WalkerIndex, float& OutDistance) const
{
	const float SplineLength = Route->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER || WalkSpeed <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float WalkDuration = SplineLength / WalkSpeed;
	float CycleDuration = WalkDuration + RespawnInterval;

	// 複数体の時は、全員が間隔WalkerSpacingで出発しきるまで次の周を始めない（等間隔に流れる）
	const int32 Count = Walkers.Num();
	if (Count > 1)
	{
		CycleDuration = FMath::Max(CycleDuration, Count * WalkerSpacing);
	}

	// 位置は時間だけで決まる（状態を持たない）。StartOffsetでアクターごと、WalkerSpacingで個体ごとにサイクルをずらす
	const float Elapsed = GetWorld()->GetTimeSeconds() + StartOffset - WalkerIndex * WalkerSpacing;
	const float CycleTime = FMath::Fmod(Elapsed, CycleDuration);
	const float PositiveCycleTime = CycleTime < 0.0f ? CycleTime + CycleDuration : CycleTime;
	if (PositiveCycleTime >= WalkDuration)
	{
		return false;
	}

	OutDistance = PositiveCycleTime * WalkSpeed;
	return true;
}

void AAmbientWalkerNPC::PlaceMesh(int32 WalkerIndex, float Distance, float DeltaSeconds)
{
	FWalkerRuntime& Walker = Walkers[WalkerIndex];
	FVector Location = Route->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

	if (bSnapToGround)
	{
		// 約0.1秒ごとに、スプライン上の位置の真下へトレースして地面の高さを得る（Landscape・道路メッシュ等のWorldStaticのみ対象）
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - Walker.LastGroundTraceTime >= AmbientWalkerGroundTraceInterval)
		{
			Walker.LastGroundTraceTime = Now;

			FHitResult Hit;
			const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AmbientWalkerGround), false, this);
			const FVector TraceStart(Location.X, Location.Y, Location.Z + GroundTraceUp);
			const FVector TraceEnd(Location.X, Location.Y, Location.Z - GroundTraceDown);
			Walker.bGroundHit = GetWorld()->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, FCollisionObjectQueryParams(ECC_WorldStatic), QueryParams);
			if (Walker.bGroundHit)
			{
				Walker.TargetGroundZ = Hit.ImpactPoint.Z;
			}
		}

		if (Walker.bGroundHit)
		{
			// 表示の開始時は即座に地面へ合わせ、以降は段差で急に上下しないようなめらかに追従する
			Walker.CurrentGroundZ = Walker.bHasCurrentGroundZ
				? FMath::FInterpTo(Walker.CurrentGroundZ, Walker.TargetGroundZ, DeltaSeconds, AmbientWalkerGroundInterpSpeed)
				: Walker.TargetGroundZ;
			Walker.bHasCurrentGroundZ = true;
			Location.Z = Walker.CurrentGroundZ;
		}
		else
		{
			// 地面に当たらない間はスプラインの高さのまま歩き、次に当たった時は補間せず即座に合わせる
			Walker.bHasCurrentGroundZ = false;
		}
	}

	const FVector Direction = Route->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	const FRotator Rotation(0.0f, Direction.Rotation().Yaw + MeshYawOffset, 0.0f);

	Walker.MeshComp->SetWorldLocationAndRotation(Location, Rotation);
}

void AAmbientWalkerNPC::SetWalkerActive(int32 WalkerIndex, bool bNewActive)
{
	FWalkerRuntime& Walker = Walkers[WalkerIndex];
	if (Walker.bActive == bNewActive)
	{
		return;
	}

	Walker.bActive = bNewActive;
	ActiveWalkerCount += bNewActive ? 1 : -1;

	if (!bNewActive)
	{
		// 止まる時に地面の状態を捨てる。次に表示される時は、最初のPlaceMeshで必ずトレースして地面の高さへ即座に合わせる
		Walker.bGroundHit = false;
		Walker.bHasCurrentGroundZ = false;
		Walker.LastGroundTraceTime = -1000.0f;
	}

	Walker.MeshComp->SetVisibility(bNewActive);
	SetActorTickEnabled(ActiveWalkerCount > 0);
}

void AAmbientWalkerNPC::UpdateActivation()
{
	// ゲーム内時間が時間帯の外なら、全員を消す（時間帯に入れば、時間から計算した位置で歩き始める）
	bool bInTimeWindow = true;
	if (ActiveTimeWindow.bEnabled)
	{
		if (const UMyProject1GameInstance* GameInst = GetGameInstance<UMyProject1GameInstance>())
		{
			bInTimeWindow = ActiveTimeWindow.IsActiveAt(GameInst->CurrentTimeInMinutes);
		}
	}

	// ロード中などプレイヤーが居ない間は誰も出さない
	const APawn* PlayerPawn = bInTimeWindow ? UGameplayStatics::GetPlayerPawn(this, 0) : nullptr;

	for (int32 Index = 0; Index < Walkers.Num(); ++Index)
	{
		float Distance = 0.0f;
		if (!PlayerPawn || !EvaluateWalk(Index, Distance))
		{
			SetWalkerActive(Index, false);
			continue;
		}

		// プレイヤーが近いか。ルートが長くても判定できるよう、NPC自身の現在位置との距離で見る
		const FVector WalkerLocation = Route->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		const bool bNear = FVector::DistSquared(WalkerLocation, PlayerPawn->GetActorLocation()) <= FMath::Square(ActiveDistance);
		if (bNear && !Walkers[Index].bActive)
		{
			// 表示する前に正しい位置へ置く（古い位置が1フレーム見えるのを防ぐ）。表示中はTickが更新するので不要
			PlaceMesh(Index, Distance, 0.0f);
		}
		SetWalkerActive(Index, bNear);
	}
}
