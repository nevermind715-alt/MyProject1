#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsChainActor.generated.h"

UCLASS()
class MYPROJECT1_API APhysicsChainActor : public AActor
{
    GENERATED_BODY()

public:
    APhysicsChainActor();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings")
    UStaticMesh* ChainLinkMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings")
    FVector ChainScale;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings")
    int32 LinkCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings")
    float LinkOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings|Limits")
    float SwingLimitAngle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings|Limits")
    float TwistLimitAngle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings|Offset")
    FVector FirstLinkLocationOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings|Offset")
    FRotator FirstLinkRotationOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings|Offset")
    FVector SecondLinkLocationOffset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain Settings|Offset")
    FRotator SecondLinkRotationOffset;

    UPROPERTY()
    USceneComponent* DeferredTargetComponent;
    FName DeferredSocketName;
    bool bPendingAttach;

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Chain Functions")
    void GenerateChain();

    UFUNCTION(BlueprintCallable, Category = "Chain Functions")
    void AttachChainStart(USceneComponent* InParentComponent, FName SocketName);

    UFUNCTION(BlueprintCallable, Category = "Chain Functions")
    void AttachChainEnd(USceneComponent* TargetComponent, FName TargetSocketName);

    void CompleteAttachEnd(USceneComponent* TargetComponent, FName TargetSocketName);

    UFUNCTION(BlueprintCallable, Category = "Chain Functions")
    UStaticMeshComponent* GetLastLink() const
    {
        if (GeneratedLinks.Num() > 0) return GeneratedLinks.Last();
        return nullptr;
    }

    UFUNCTION(BlueprintCallable, Category = "Chain Functions")
    UStaticMeshComponent* AttachMeshToLastLink(UStaticMesh* MeshToAttach, FVector Scale = FVector(1.0f));

private:
    UPROPERTY()
    TArray<UStaticMeshComponent*> GeneratedLinks;

    UPROPERTY()
    TArray<UPhysicsConstraintComponent*> GeneratedConstraints;
};