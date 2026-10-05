#include "SleepPoint.h"
#include "Components/StaticMeshComponent.h"
#include "MyProject1Character.h"
#include "MyProject1Types.h"
#include "EventDistributorComponent.h"
#include "MyProject1GameInstance.h"
#include "RpgCharacterInterface.h"

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
	if (Interactor->TryOpenTimeSkipMenu(/*bIsSleepMode=*/true))
	{
		// メニューを開けた（＝これから眠る可能性がある）場合のみ、どのSleepPointで眠ろうとしているかを記憶させる。
		// 実際に眠り終えたタイミングでGameInstance::ExecuteWarpProcessがこれを見てTriggerSleepEventPoolIfEnabledを呼ぶ
		Interactor->SetPendingSleepPointContext(this);

		// GetPreSleepInteractTransform用。bUseContextActorLocationInsteadOfWarpでこのSleepPointへワープした
		// 睡眠イベントの戻り先として、ResolveActiveEventがこの座標を参照する
		PreSleepInteractTransform = Interactor->GetActorTransform();
	}
	else
	{
		Interactor->OnReceiveLogMessage(TEXT("今は眠れないようだ。"), ELogMessageType::System);
	}
}

bool ASleepPoint::TriggerSleepEventPoolIfEnabled(AMyProject1Character* PlayerCharacter)
{
	PendingSleepEventID = NAME_None;
	if (!bTriggerEventPoolOnSleep || !PlayerCharacter) return false;

	UEventDistributorComponent* Distributor = FindComponentByClass<UEventDistributorComponent>();
	if (!Distributor) return false;

	PendingSleepEventID = Distributor->RollEventPool(PlayerCharacter);
	return !PendingSleepEventID.IsNone();
}

void ASleepPoint::BeginPendingSleepEvent(AMyProject1Character* PlayerCharacter)
{
	if (!PlayerCharacter) return;

	if (!EventEncounterNarrationText.IsEmpty())
	{
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(PlayerCharacter->GetGameInstance()))
		{
			// 通常の会話開始時と同じく、セリフ表示中は入力をロックする（解除はOnSleepEventNarrationFinishedで行う）
			if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerCharacter))
			{
				RpgInterface->SetInputLocked(true);
			}

			GameInst->BeginSleepEventNarration(EventEncounterNarrationText, EventEncounterNarrationDisplaySeconds, this, PlayerCharacter);
			return;
		}
	}

	// セリフが設定されていない場合は、次のTickでイベントを発動する。このBeginPendingSleepEvent自体が
	// まだ暗転完了処理（ExecuteWarpProcess）のコールスタック中から呼ばれているため、その場でStartEvent
	// （ワープ等）を呼ぶと暗転処理が二重に走る危険があるのを避ける
	TWeakObjectPtr<AMyProject1Character> WeakPlayer = PlayerCharacter;
	GetWorld()->GetTimerManager().SetTimerForNextTick([this, WeakPlayer]()
		{
			if (AMyProject1Character* DeferredPlayer = WeakPlayer.Get())
			{
				StartPendingSleepEvent(DeferredPlayer);
			}
		});
}

void ASleepPoint::OnSleepEventNarrationFinished(AMyProject1Character* PlayerCharacter)
{
	if (!PlayerCharacter) return;

	// BeginPendingSleepEventで立てた入力ロックを、通常の会話終了（CloseDialog）と同じ考え方で解除する
	if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerCharacter))
	{
		RpgInterface->SetInputLocked(false);
	}

	StartPendingSleepEvent(PlayerCharacter);
}

void ASleepPoint::StartPendingSleepEvent(AMyProject1Character* PlayerCharacter)
{
	const FName EventIDToStart = PendingSleepEventID;
	PendingSleepEventID = NAME_None;

	UMyProject1GameInstance* GameInst = PlayerCharacter ? Cast<UMyProject1GameInstance>(PlayerCharacter->GetGameInstance()) : nullptr;
	if (!GameInst) return;

	if (!EventIDToStart.IsNone())
	{
		GameInst->StartEvent(EventIDToStart, PlayerCharacter, this);
	}

	// ClearCondition=Instant（またはEventIDが空欄）の場合はStartEvent内で即座に完了しているため、
	// ここで明転を再開する。Warp/AnimationSequence等を経由した場合はbHasActiveEventがtrueになっており、
	// それ以降のフェード管理は既存のResolveActiveEvent側に委ねるため、ここでは何もしない
	if (!GameInst->bHasActiveEvent)
	{
		GameInst->ResumeFadeInAfterSleepEvent();
	}
}
