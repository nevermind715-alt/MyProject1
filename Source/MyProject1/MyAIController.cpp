#include "MyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject1Character.h" 
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"

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

    // 1. キャラクター側の設定値をAIの「目」に反映する
    if (MyChar && SightConfig)
    {
        SightConfig->SightRadius = MyChar->AISightRadius;
        SightConfig->LoseSightRadius = MyChar->AILoseSightRadius;
        SightConfig->PeripheralVisionAngleDegrees = MyChar->AIVisionAngle;

        // 設定を更新
        PerceptionComp->ConfigureSense(*SightConfig);
    }

    // 2. キャラクター側の設定値をAIの「耳」に反映する
    if (MyChar && HearingConfig)
    {
        if (MyChar->bAIEnableHearing)
        {
            // 有効なら設定された範囲を適用
            HearingConfig->HearingRange = MyChar->AIHearingRange;
        }
        else
        {
            // 無効なら範囲を0にして聞こえなくする
            HearingConfig->HearingRange = 0.0f;
        }

        // 聴覚設定を更新して反映
        PerceptionComp->ConfigureSense(*HearingConfig);
    }

    // 2. ビヘイビアツリーを実行 & 初期設定
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