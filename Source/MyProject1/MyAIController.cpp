#include "MyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NPCAIInterface.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

AMyAIController::AMyAIController()
{
    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2000.0f;
    SightConfig->PeripheralVisionAngleDegrees = 90.0f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

    PerceptionComp->ConfigureSense(*SightConfig);

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 3000.0f;
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    PerceptionComp->ConfigureSense(*HearingConfig);

    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnTargetDetected);
}

void AMyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    INPCAIInterface* AIPawn = Cast<INPCAIInterface>(InPawn);
    if (!AIPawn) return;

    ACharacter* MyChar = Cast<ACharacter>(InPawn);

    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);

        UBlackboardComponent* BB = GetBlackboardComponent();
        if (BB)
        {
            BB->SetValueAsVector(TEXT("HomeLocation"), InPawn->GetActorLocation());

            BB->SetValueAsBool(TEXT("CanPatrol"), AIPawn->CanPatrol());

            if (MyChar && MyChar->GetCharacterMovement())
            {
                MyChar->GetCharacterMovement()->MaxWalkSpeed = AIPawn->GetPatrolWalkSpeed();
            }
        }
    }

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AMyAIController::ApplyPerceptionSettings, 0.1f, false);
   
}


void AMyAIController::ApplyPerceptionSettings()
{
    INPCAIInterface* AIPawn = Cast<INPCAIInterface>(GetPawn());
    if (!AIPawn) return;

    FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
    UAISenseConfig_Sight* ActiveSight = Cast<UAISenseConfig_Sight>(PerceptionComp->GetSenseConfig(SightID));

    if (ActiveSight)
    {
        ActiveSight->SightRadius = AIPawn->GetAISightRadius();
        ActiveSight->LoseSightRadius = AIPawn->GetAILoseSightRadius();
        ActiveSight->PeripheralVisionAngleDegrees = AIPawn->GetAIVisionAngle();

        PerceptionComp->ConfigureSense(*ActiveSight);
    }

    FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
    UAISenseConfig_Hearing* ActiveHearing = Cast<UAISenseConfig_Hearing>(PerceptionComp->GetSenseConfig(HearingID));

    if (ActiveHearing)
    {
        ActiveHearing->HearingRange = AIPawn->IsAIHearingEnabled() ? AIPawn->GetAIHearingRange() : 0.0f;

        PerceptionComp->ConfigureSense(*ActiveHearing);
    }

    PerceptionComp->RequestStimuliListenerUpdate();
}

void AMyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor->ActorHasTag(TEXT("Player")))
    {
        return;
    }

    INPCAIInterface* AIPawn = Cast<INPCAIInterface>(GetPawn());
    ACharacter* MyChar = Cast<ACharacter>(GetPawn());
    UCharacterMovementComponent* MoveComp = MyChar ? MyChar->GetCharacterMovement() : nullptr;
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (!BB) return;

    if (Stimulus.WasSuccessfullySensed())
    {
        if (AIPawn && !AIPawn->IsActiveEnemy())
        {
            AActor* CurrentBBTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
            if (CurrentBBTarget != Actor)
            {
                return;
            }
        }

        BB->SetValueAsObject(TEXT("TargetActor"), Actor);

        if (AIPawn)
        {
            if (MoveComp)
            {
                MoveComp->MaxWalkSpeed = AIPawn->GetChaseRunSpeed();
            }
            AIPawn->SetCurrentTarget(Actor);
        }
    }
    else
    {
        if (AIPawn && AIPawn->GetNeverLoseSight()) return;

        float Dist = GetPawn()->GetDistanceTo(Actor);
        if (AIPawn && Dist < AIPawn->GetAILoseSightRadius())
        {
            return;
        }

        if (BB->GetValueAsObject(TEXT("TargetActor")) == Actor)
        {
            BB->SetValueAsObject(TEXT("TargetActor"), nullptr);

            ClearFocus(EAIFocusPriority::Gameplay);

            if (AIPawn)
            {
                AIPawn->SetCurrentTarget(nullptr);
            }

            if (MoveComp && AIPawn)
            {
                MoveComp->MaxWalkSpeed = AIPawn->GetPatrolWalkSpeed();
            }
        }
    }
}

void AMyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB) return;

    AActor* Target = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
    if (!Target) return;

    INPCAIInterface* TargetAI = Cast<INPCAIInterface>(Target);
    if (TargetAI && TargetAI->IsDead())
    {

        bool bHasWon = BB->GetValueAsBool(TEXT("HasWon"));
        if (!bHasWon)
        {
            BB->SetValueAsBool(TEXT("HasWon"), true);

            StopMovement();

            INPCAIInterface* AIPawn = Cast<INPCAIInterface>(GetPawn());
            ACharacter* MyChar = Cast<ACharacter>(GetPawn());
            if (AIPawn)
            {
                if (MyChar && MyChar->GetCharacterMovement())
                {
                    MyChar->GetCharacterMovement()->MaxWalkSpeed = AIPawn->GetPatrolWalkSpeed();
                }

                AIPawn->SetCurrentTarget(nullptr);
            }

        }
    }
}
