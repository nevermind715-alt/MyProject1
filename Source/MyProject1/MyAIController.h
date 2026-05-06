#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h" // ★追加：知覚情報の型
#include "MyAIController.generated.h"

// 前方宣言（クラスがあることだけ伝える）
class UBehaviorTree;
class UBlackboardComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing; // 聴覚用（準備）

UCLASS()
class MYPROJECT1_API AMyAIController : public AAIController
{
	GENERATED_BODY()

public:
	// コンストラクタの宣言（これが抜けていた可能性があります）
	AMyAIController();

protected:
	// Possess（憑依）時の処理の宣言（これも抜けていた可能性があります）
	virtual void OnPossess(APawn* InPawn) override;

	// ビヘイビアツリーのアセット
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

    // --- 知覚コンポーネント ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* PerceptionComp;

    // 視覚の設定
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAISenseConfig_Sight* SightConfig;

    // 聴覚の設定（今回は準備だけ）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAISenseConfig_Hearing* HearingConfig;

    /** 何かを見たり聞いたりした時に呼ばれる関数 */
    UFUNCTION()
    void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

public:
   
    //毎フレーム実行する関数（生存確認用）
    virtual void Tick(float DeltaTime) override;

    //遅延させて視界設定を適用するための関数
    UFUNCTION()
    void ApplyPerceptionSettings();

    // ... (OnPossess など他の関数) ...
};