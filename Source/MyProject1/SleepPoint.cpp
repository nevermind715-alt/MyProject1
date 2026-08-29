#include "SleepPoint.h"
#include "Components/StaticMeshComponent.h"
#include "MyProject1Character.h"
#include "MyProject1Types.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

ASleepPoint::ASleepPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// TargetNearestEnemy()のターゲット判定はActorのTag（"Enemy"か"NPC"）を見ているだけなので、
	// これを付けておくだけでNPCと同じ射程判定(InteractRange)に自動的に乗る
	Tags.Add(FName("NPC"));
}

void ASleepPoint::TryInteract(AMyProject1Character* Interactor)
{
	if (!Interactor) return;

	// 成功・失敗を問わず、インタラクトした時点でターゲットカーソルを消す
	Interactor->CancelTarget();

	// 会話中・カットシーン中・戦闘中はTryOpenTimeSkipMenu側のガードでfalseが返る
	if (!Interactor->TryOpenTimeSkipMenu(/*bIsSleepMode=*/true))
	{
		Interactor->OnReceiveLogMessage(TEXT("今は眠れないようだ。"), ELogMessageType::System);
	}
}
