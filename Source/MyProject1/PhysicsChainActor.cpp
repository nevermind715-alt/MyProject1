#include "PhysicsChainActor.h"

APhysicsChainActor::APhysicsChainActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bPendingAttach = false;

    LinkCount = 8;
    LinkOffset = 40.0f;
    SwingLimitAngle = 45.0f;
    TwistLimitAngle = 20.0f;
    ChainLinkMesh = nullptr;
    ChainScale = FVector(1.0f, 1.0f, 1.0f);

    FirstLinkLocationOffset = FVector::ZeroVector;
    FirstLinkRotationOffset = FRotator::ZeroRotator;
    SecondLinkLocationOffset = FVector::ZeroVector;
    SecondLinkRotationOffset = FRotator::ZeroRotator;

    USceneComponent* DummyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    DummyRoot->SetMobility(EComponentMobility::Movable);
    RootComponent = DummyRoot;
}

void APhysicsChainActor::BeginPlay()
{
    Super::BeginPlay();
}

void APhysicsChainActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bPendingAttach && DeferredTargetComponent)
    {
        FVector SocketLoc = DeferredTargetComponent->GetSocketLocation(DeferredSocketName);
        if (!SocketLoc.IsNearlyZero())
        {
            CompleteAttachEnd(DeferredTargetComponent, DeferredSocketName);
            bPendingAttach = false;
        }
    }
}

void APhysicsChainActor::GenerateChain()
{
    if (!ChainLinkMesh) return;

    for (UStaticMeshComponent* Link : GeneratedLinks) { if (Link) Link->DestroyComponent(); }
    for (UPhysicsConstraintComponent* Constraint : GeneratedConstraints) { if (Constraint) Constraint->DestroyComponent(); }
    GeneratedLinks.Empty();
    GeneratedConstraints.Empty();

    for (int32 i = 0; i < LinkCount; ++i)
    {
        FName MeshName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("ChainLink"));
        UStaticMeshComponent* NewMesh = NewObject<UStaticMeshComponent>(this, MeshName);
        NewMesh->CreationMethod = EComponentCreationMethod::Instance;
        NewMesh->SetupAttachment(RootComponent);

        bool bIsEven = (i % 2 == 0);
        FVector LocalLocation = FVector(0.0f, 0.0f, -(static_cast<float>(i) * LinkOffset));
        FRotator LocalRotation = bIsEven ? FRotator(-90.0f, 0.0f, 0.0f) + FirstLinkRotationOffset : FRotator(-90.0f, 0.0f, 90.0f) + SecondLinkRotationOffset;

        FVector LocationOffset = bIsEven ? FirstLinkLocationOffset : SecondLinkLocationOffset;
        NewMesh->SetRelativeLocationAndRotation(LocalLocation + LocationOffset, LocalRotation);
        NewMesh->SetRelativeScale3D(ChainScale);
        NewMesh->SetStaticMesh(ChainLinkMesh);

        // ★修正: 物理シミュレーションしつつ、キャラクター（Pawn）との衝突を無視する
        NewMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
        NewMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        NewMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

        NewMesh->RegisterComponent();
        GeneratedLinks.Add(NewMesh);
    }

    // (以下、Constraintと物理ONの設定は変更なし)
    for (int32 i = 1; i < GeneratedLinks.Num(); ++i)
    {
        UPhysicsConstraintComponent* NewConstraint = NewObject<UPhysicsConstraintComponent>(this);
        NewConstraint->SetupAttachment(RootComponent);
        NewConstraint->RegisterComponent();
        NewConstraint->SetWorldLocation((GeneratedLinks[i - 1]->GetComponentLocation() + GeneratedLinks[i]->GetComponentLocation()) * 0.5f);
        NewConstraint->SetConstrainedComponents(GeneratedLinks[i - 1], NAME_None, GeneratedLinks[i], NAME_None);
        NewConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
        NewConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
        NewConstraint->SetLinearZLimit(LCM_Locked, 0.0f);
        NewConstraint->SetAngularSwing1Limit(ACM_Limited, SwingLimitAngle);
        NewConstraint->SetAngularSwing2Limit(ACM_Limited, SwingLimitAngle);
        NewConstraint->SetAngularTwistLimit(ACM_Limited, TwistLimitAngle);
        NewConstraint->InitComponentConstraint();
        GeneratedConstraints.Add(NewConstraint);
    }

    for (int32 i = 1; i < GeneratedLinks.Num(); ++i)
    {
        GeneratedLinks[i]->SetSimulatePhysics(true);
        GeneratedLinks[i]->WakeRigidBody();
    }
}

void APhysicsChainActor::AttachChainStart(USceneComponent* InParentComponent, FName SocketName)
{
    if (!InParentComponent || !InParentComponent->DoesSocketExist(SocketName)) return;
    this->AttachToComponent(InParentComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    GenerateChain();
}

void APhysicsChainActor::AttachChainEnd(USceneComponent* TargetComponent, FName TargetSocketName)
{
    DeferredTargetComponent = TargetComponent;
    DeferredSocketName = TargetSocketName;
    bPendingAttach = true;
}

void APhysicsChainActor::CompleteAttachEnd(USceneComponent* TargetComponent, FName TargetSocketName)
{
    if (GeneratedLinks.Num() == 0) return;
    UStaticMeshComponent* LastLink = GeneratedLinks.Last();

    UStaticMeshComponent* EndAnchor = NewObject<UStaticMeshComponent>(this);
    EndAnchor->SetStaticMesh(ChainLinkMesh);
    EndAnchor->SetupAttachment(TargetComponent, TargetSocketName);
    EndAnchor->RegisterComponent();

    EndAnchor->SetSimulatePhysics(true);
    EndAnchor->SetEnableGravity(false);
    EndAnchor->SetMassOverrideInKg(NAME_None, 1.0f, true);

    // ★修正: こちらもキャラクターとの衝突を無視する
    EndAnchor->SetCollisionProfileName(TEXT("PhysicsBody"));
    EndAnchor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    EndAnchor->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    UPhysicsConstraintComponent* EndConstraint = NewObject<UPhysicsConstraintComponent>(this);
    EndConstraint->SetupAttachment(RootComponent);
    EndConstraint->RegisterComponent();

    EndConstraint->SetWorldLocation(TargetComponent->GetSocketLocation(TargetSocketName));
    EndConstraint->SetConstrainedComponents(LastLink, NAME_None, EndAnchor, NAME_None);

    EndConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
    EndConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
    EndConstraint->SetLinearZLimit(LCM_Locked, 0.0f);

    EndConstraint->InitComponentConstraint();

    GeneratedLinks.Add(EndAnchor);
    GeneratedConstraints.Add(EndConstraint);
}

UStaticMeshComponent* APhysicsChainActor::AttachMeshToLastLink(UStaticMesh* MeshToAttach, FVector Scale)
{
    if (!MeshToAttach || GeneratedLinks.Num() == 0) return nullptr;
    UStaticMeshComponent* EndMeshComp = NewObject<UStaticMeshComponent>(this);
    EndMeshComp->SetupAttachment(GeneratedLinks.Last());
    EndMeshComp->RegisterComponent();
    EndMeshComp->SetStaticMesh(MeshToAttach);
    EndMeshComp->SetRelativeScale3D(Scale);
    EndMeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -LinkOffset));
    return EndMeshComp;
}