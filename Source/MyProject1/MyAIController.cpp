#include "MyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject1Character.h" 
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

// コンストラクタ
AMyAIController::AMyAIController()
{
    // 1. 知覚コンポーネントの作成
    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));

    // 2. 視覚の設定を作成
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2000.0f;
    SightConfig->PeripheralVisionAngleDegrees = 90.0f;
    // 「敵・中立・味方」全部に反応させる設定
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

    // コンポーネントに視覚設定を登録
    PerceptionComp->ConfigureSense(*SightConfig);

    // 3. 聴覚の設定（準備）
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 3000.0f;
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    PerceptionComp->ConfigureSense(*HearingConfig);

    // 視覚を優先センスに設定
    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

    // 4. イベント登録
    PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AMyAIController::OnTargetDetected);
}

// 憑依（Possess）した時に呼ばれる関数
void AMyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    AMyProject1Character* MyChar = Cast<AMyProject1Character>(InPawn);
    if (!MyChar) return;

    // 1. ビヘイビアツリーを実行 & 初期設定
    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);

        UBlackboardComponent* BB = GetBlackboardComponent();
        if (BB && MyChar)
        {
            // 「家（Home）」の位置を記憶
            BB->SetValueAsVector(TEXT("HomeLocation"), MyChar->GetActorLocation());

            // 「パトロールするか？」の設定をコピー
            BB->SetValueAsBool(TEXT("CanPatrol"), MyChar->bCanPatrol);

            // 最初は「歩き速度」に設定する
            if (MyChar->GetCharacterMovement())
            {
                MyChar->GetCharacterMovement()->MaxWalkSpeed = MyChar->PatrolWalkSpeed;
            }
        }
    }

    //スポーン直後の0.1秒だけ待ってから視界を更新する！
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AMyAIController::ApplyPerceptionSettings, 0.1f, false);
   
}


//視界・聴覚の設定を実際に上書きする処理
void AMyAIController::ApplyPerceptionSettings()
{
    AMyProject1Character* MyChar = Cast<AMyProject1Character>(GetPawn());
    if (!MyChar) return;

    // 1. AIの脳内から「現在アクティブな視覚設定」を取り出して上書きする
    FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
    UAISenseConfig_Sight* ActiveSight = Cast<UAISenseConfig_Sight>(PerceptionComp->GetSenseConfig(SightID));

    if (ActiveSight)
    {
        ActiveSight->SightRadius = MyChar->AISightRadius;
        ActiveSight->LoseSightRadius = MyChar->AILoseSightRadius;
        ActiveSight->PeripheralVisionAngleDegrees = MyChar->AIVisionAngle;

        PerceptionComp->ConfigureSense(*ActiveSight);
    }

    // 2. 「現在アクティブな聴覚設定」を取り出して上書きする
    FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
    UAISenseConfig_Hearing* ActiveHearing = Cast<UAISenseConfig_Hearing>(PerceptionComp->GetSenseConfig(HearingID));

    if (ActiveHearing)
    {
        ActiveHearing->HearingRange = MyChar->bAIEnableHearing ? MyChar->AIHearingRange : 0.0f;

        PerceptionComp->ConfigureSense(*ActiveHearing);
    }

    // 3. コンポーネント自身に「設定が変わったから今すぐ反映して！」と命令する
    PerceptionComp->RequestStimuliListenerUpdate();
}

// ターゲットを発見・見失った時の処理
void AMyAIController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
    // 「Player」タグを持っていない相手は無視する
    if (!Actor->ActorHasTag(TEXT("Player")))
    {
        return;
    }

    AMyProject1Character* MyChar = Cast<AMyProject1Character>(GetPawn());
    UCharacterMovementComponent* MoveComp = MyChar ? MyChar->GetCharacterMovement() : nullptr;
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (!BB) return;

    // --- パターンA: 発見した時 ---
    if (Stimulus.WasSuccessfullySensed())
    {
        BB->SetValueAsObject(TEXT("TargetActor"), Actor);

        if (MyChar)
        {
            // ★追加：見つけた瞬間に速度を ChaseRunSpeed（走り）に切り替える
            if (MoveComp)
            {
                MoveComp->MaxWalkSpeed = MyChar->ChaseRunSpeed;
            }
            MyChar->SetCurrentTarget(Actor);
        }
    }
    // --- パターンB: 見失った時 ---
    else
    {
        // 1. ボスモードなら無視
        if (MyChar && MyChar->bNeverLoseSight) return;

        // 2. 距離チェック：見えなくなっても、距離が近ければ諦めない
        float Dist = GetPawn()->GetDistanceTo(Actor);
        if (MyChar && Dist < MyChar->AILoseSightRadius)
        {
            return; // まだ近いのでターゲット維持
        }

        // 3. 本当に諦める時
        if (BB->GetValueAsObject(TEXT("TargetActor")) == Actor)
        {
            BB->SetValueAsObject(TEXT("TargetActor"), nullptr);

            // ロックオン解除
            ClearFocus(EAIFocusPriority::Gameplay);

            // 体のターゲットも解除する (攻撃中止)
            if (MyChar)
            {
                MyChar->SetCurrentTarget(nullptr);
            }

            // 諦めたら「歩き速度」に戻す
            if (MoveComp && MyChar)
            {
                MoveComp->MaxWalkSpeed = MyChar->PatrolWalkSpeed;
            }
        }
    }
}

// 毎フレーム実行される（監視役）
void AMyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB) return;

    // 1. 今のターゲットを取得
    AActor* Target = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
    if (!Target) return;

    // 2. ターゲットが「死んでいるか」チェック
    AMyProject1Character* TargetChar = Cast<AMyProject1Character>(Target);
    if (TargetChar && TargetChar->IsDead())
    {
        // --- ★ここを修正：死んだら「勝利モード」へ移行 ---

        // すでに勝利フラグが立っていれば何もしない
        bool bHasWon = BB->GetValueAsBool(TEXT("HasWon"));
        if (!bHasWon)
        {
            // A. 「勝ったよフラグ」を立てる（これでBTが勝利演出に入る）
            BB->SetValueAsBool(TEXT("HasWon"), true);

            // B. 動きを止める（死体を押さないように）
            StopMovement();

            // C. 速度を「歩き」に戻しておく（次に動き出す時のため）
            AMyProject1Character* MyChar = Cast<AMyProject1Character>(GetPawn());
            if (MyChar)
            {
                if (MyChar->GetCharacterMovement())
                {
                    MyChar->GetCharacterMovement()->MaxWalkSpeed = MyChar->PatrolWalkSpeed;
                }

                // 攻撃システム（体のターゲット）は解除しておく
                MyChar->SetCurrentTarget(nullptr);
            }

            // ※ここでは TargetActor を消しません！
            // 消すとBTが即座にパトロールに戻ってしまうため、BT側のWaitタスクで消すのを待ちます。
        }
    }
}