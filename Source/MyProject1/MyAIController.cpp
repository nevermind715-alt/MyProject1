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

// �R���X�g���N�^
AMyAIController::AMyAIController()
{
    // 1. �m�o�R���|�[�l���g�̍쐬
    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));

    // 2. ���o�̐ݒ���쐬
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2000.0f;
    SightConfig->PeripheralVisionAngleDegrees = 90.0f;
    // �u�G�E�����E�����v�S���ɔ���������ݒ�
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

    // �R���|�[�l���g�Ɏ��o�ݒ��o�^
    PerceptionComp->ConfigureSense(*SightConfig);

    // 3. ���o�̐ݒ�i�����j
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 3000.0f;
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    PerceptionComp->ConfigureSense(*HearingConfig);

    // ���o��D��Z���X�ɐݒ�
    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

    // 4. �C�x���g�o�^
    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnTargetDetected);
}

// �߈ˁiPossess�j�������ɌĂ΂��֐�
void AMyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    INPCAIInterface* AIPawn = Cast<INPCAIInterface>(InPawn);
    if (!AIPawn) return;

    ACharacter* MyChar = Cast<ACharacter>(InPawn);

    // 1. �r�w�C�r�A�c���[�����s & �����ݒ�
    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);

        UBlackboardComponent* BB = GetBlackboardComponent();
        if (BB)
        {
            // �u�ƁiHome�j�v�̈ʒu���L��
            BB->SetValueAsVector(TEXT("HomeLocation"), InPawn->GetActorLocation());

            // �u�p�g���[�����邩�H�v�̐ݒ���R�s�[
            BB->SetValueAsBool(TEXT("CanPatrol"), AIPawn->CanPatrol());

            // �ŏ��́u�������x�v�ɐݒ肷��
            if (MyChar && MyChar->GetCharacterMovement())
            {
                MyChar->GetCharacterMovement()->MaxWalkSpeed = AIPawn->GetPatrolWalkSpeed();
            }
        }
    }

    //�X�|�[�������0.1�b�����҂��Ă��王�E���X�V����I
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AMyAIController::ApplyPerceptionSettings, 0.1f, false);
   
}


//���E�E���o�̐ݒ�����ۂɏ㏑�����鏈��
void AMyAIController::ApplyPerceptionSettings()
{
    INPCAIInterface* AIPawn = Cast<INPCAIInterface>(GetPawn());
    if (!AIPawn) return;

    // 1. AI�̔]������u���݃A�N�e�B�u�Ȏ��o�ݒ�v�����o���ď㏑������
    FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
    UAISenseConfig_Sight* ActiveSight = Cast<UAISenseConfig_Sight>(PerceptionComp->GetSenseConfig(SightID));

    if (ActiveSight)
    {
        ActiveSight->SightRadius = AIPawn->GetAISightRadius();
        ActiveSight->LoseSightRadius = AIPawn->GetAILoseSightRadius();
        ActiveSight->PeripheralVisionAngleDegrees = AIPawn->GetAIVisionAngle();

        PerceptionComp->ConfigureSense(*ActiveSight);
    }

    // 2. �u���݃A�N�e�B�u�Ȓ��o�ݒ�v�����o���ď㏑������
    FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
    UAISenseConfig_Hearing* ActiveHearing = Cast<UAISenseConfig_Hearing>(PerceptionComp->GetSenseConfig(HearingID));

    if (ActiveHearing)
    {
        ActiveHearing->HearingRange = AIPawn->IsAIHearingEnabled() ? AIPawn->GetAIHearingRange() : 0.0f;

        PerceptionComp->ConfigureSense(*ActiveHearing);
    }

    // 3. �R���|�[�l���g���g�Ɂu�ݒ肪�ς�������獡�������f���āI�v�Ɩ��߂���
    PerceptionComp->RequestStimuliListenerUpdate();
}

// �^�[�Q�b�g�𔭌��E�����������̏���
void AMyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
    // �uPlayer�v�^�O�������Ă��Ȃ�����͖�������
    if (!Actor->ActorHasTag(TEXT("Player")))
    {
        return;
    }

    INPCAIInterface* AIPawn = Cast<INPCAIInterface>(GetPawn());
    ACharacter* MyChar = Cast<ACharacter>(GetPawn());
    UCharacterMovementComponent* MoveComp = MyChar ? MyChar->GetCharacterMovement() : nullptr;
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (!BB) return;

    // --- �p�^�[��A: ���������� ---
    if (Stimulus.WasSuccessfullySensed())
    {
        //�������m���A�N�e�B�u�i��A�N�e�B�u�j�Ȃ�A���E�ɓ����Ă��P��Ȃ��I
        if (AIPawn && !AIPawn->IsActiveEnemy())
        {
            // �������A���łɍU������ă^�[�Q�b�g�Ƃ��ĔF�����Ă��鑊��Ȃ�ǐՂ�������
            AActor* CurrentBBTarget = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
            if (CurrentBBTarget != Actor)
            {
                // �܂��퓬��ԂłȂ���΁A�������ċA��i�U�����Ȃ��j
                return;
            }
        }

        BB->SetValueAsObject(TEXT("TargetActor"), Actor);

        if (AIPawn)
        {
            //�������u�Ԃɑ��x�� ChaseRunSpeed�i����j�ɐ؂�ւ���
            if (MoveComp)
            {
                MoveComp->MaxWalkSpeed = AIPawn->GetChaseRunSpeed();
            }
            AIPawn->SetCurrentTarget(Actor);
        }
    }
    // --- �p�^�[��B: ���������� ---
    else
    {
        // 1. �{�X���[�h�Ȃ疳��
        if (AIPawn && AIPawn->GetNeverLoseSight()) return;

        // 2. �����`�F�b�N�F�����Ȃ��Ȃ��Ă��A�������߂���Β��߂Ȃ�
        float Dist = GetPawn()->GetDistanceTo(Actor);
        if (AIPawn && Dist < AIPawn->GetAILoseSightRadius())
        {
            return; // �܂��߂��̂Ń^�[�Q�b�g�ێ�
        }

        // 3. �{���ɒ��߂鎞
        if (BB->GetValueAsObject(TEXT("TargetActor")) == Actor)
        {
            BB->SetValueAsObject(TEXT("TargetActor"), nullptr);

            // ���b�N�I������
            ClearFocus(EAIFocusPriority::Gameplay);

            // �̂̃^�[�Q�b�g���������� (�U�����~)
            if (AIPawn)
            {
                AIPawn->SetCurrentTarget(nullptr);
            }

            // ���߂���u�������x�v�ɖ߂�
            if (MoveComp && AIPawn)
            {
                MoveComp->MaxWalkSpeed = AIPawn->GetPatrolWalkSpeed();
            }
        }
    }
}

// ���t���[�����s�����i�Ď����j
void AMyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB) return;

    // 1. ���̃^�[�Q�b�g���擾
    AActor* Target = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
    if (!Target) return;

    // 2. �^�[�Q�b�g���u����ł��邩�v�`�F�b�N
    INPCAIInterface* TargetAI = Cast<INPCAIInterface>(Target);
    if (TargetAI && TargetAI->IsDead())
    {
        // --- ���񂾂�u�������[�h�v�ֈڍs ---

        // ���łɏ����t���O�������Ă���Ή������Ȃ�
        bool bHasWon = BB->GetValueAsBool(TEXT("HasWon"));
        if (!bHasWon)
        {
            // A. �u��������t���O�v�𗧂Ă�i�����BT���������o�ɓ���j
            BB->SetValueAsBool(TEXT("HasWon"), true);

            // B. �������~�߂�i���̂������Ȃ��悤�Ɂj
            StopMovement();

            // C. ���x���u�����v�ɖ߂��Ă����i���ɓ����o�����̂��߁j
            INPCAIInterface* AIPawn = Cast<INPCAIInterface>(GetPawn());
            ACharacter* MyChar = Cast<ACharacter>(GetPawn());
            if (AIPawn)
            {
                if (MyChar && MyChar->GetCharacterMovement())
                {
                    MyChar->GetCharacterMovement()->MaxWalkSpeed = AIPawn->GetPatrolWalkSpeed();
                }

                // �U���V�X�e���i�̂̃^�[�Q�b�g�j�͉������Ă���
                AIPawn->SetCurrentTarget(nullptr);
            }

            // �������ł� TargetActor �������܂���I
            // ������BT�������Ƀp�g���[���ɖ߂��Ă��܂����߁ABT����Wait�^�X�N�ŏ����̂�҂��܂��B
        }
    }
}