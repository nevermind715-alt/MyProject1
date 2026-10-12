#include "WallWarpLink.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "MyProject1Character.h"
#include "MyProject1GameInstance.h"
#include "Kismet/GameplayStatics.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

AWallWarpLink::AWallWarpLink()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	CollisionBox->InitBoxExtent(FVector(50.f, 50.f, 100.f));

	// プリセット名（"Trigger"）任せにせず、どのチャンネルも絶対にブロックしないことをここで明示する。
	// このプロジェクトはPlayer/Enemy等の独自コリジョンチャンネルを追加しているため（DefaultEngine.ini）、
	// PawnチャンネルだけをOverlap指定すると、実際にプレイヤーが使っているチャンネルには反応せず
	// Overlapイベントが発火しなくなる。未知のカスタムチャンネルがあっても確実に動くよう、全チャンネルOverlapにする
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionBox->SetGenerateOverlapEvents(true);

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWallWarpLink::OnOverlapBegin);
	CollisionBox->OnComponentEndOverlap.AddDynamic(this, &AWallWarpLink::OnOverlapEnd);
}

void AWallWarpLink::BeginPlay()
{
	Super::BeginPlay();

	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerChar) return;

	UpdateFlagVisibility(PlayerChar);

	// RequiredFlagが空欄でも購読だけはしておいて問題ない（OnPlayerFlagAdded側で無視される）
	PlayerChar->OnFlagAdded.AddDynamic(this, &AWallWarpLink::OnPlayerFlagAdded);
	PlayerChar->OnFlagRemoved.AddDynamic(this, &AWallWarpLink::OnPlayerFlagRemoved);

	if (bAutoClearFlag && !RequiredFlag.IsNone())
	{
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
		{
			GameInst->OnInGameTimeChanged.AddDynamic(this, &AWallWarpLink::OnInGameTimeChangedHandler);
		}

		// すでにフラグを持った状態で始まった場合（レベル移動後・ロード後）。期限の記録が無ければ今から数え始め、
		// 期限が過ぎていればその場で外す
		if (PlayerChar->HasFlag(RequiredFlag))
		{
			StartAutoClearDeadline();
			CheckAutoClearDeadline();
		}
	}
}

void AWallWarpLink::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		GameInst->OnInGameTimeChanged.RemoveDynamic(this, &AWallWarpLink::OnInGameTimeChangedHandler);
	}

	Super::EndPlay(EndPlayReason);
}

void AWallWarpLink::StartAutoClearDeadline()
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst || GameInst->FlagAutoClearDeadlines.Contains(RequiredFlag)) return;

	const int32 NowMinutes = GameInst->TotalElapsedDays * 1440 + GameInst->CurrentTimeInMinutes;
	GameInst->FlagAutoClearDeadlines.Add(RequiredFlag, NowMinutes + FMath::RoundToInt(AutoClearFlagGameHours * 60.0f));
}

void AWallWarpLink::CheckAutoClearDeadline()
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	const int32* Deadline = GameInst->FlagAutoClearDeadlines.Find(RequiredFlag);
	if (!Deadline) return;

	const int32 NowMinutes = GameInst->TotalElapsedDays * 1440 + GameInst->CurrentTimeInMinutes;
	if (NowMinutes < *Deadline) return;

	// 期限記録の削除はOnPlayerFlagRemoved側でも行うが、プレイヤーが取れない場合に毎分ここへ来続けないよう先に消す
	GameInst->FlagAutoClearDeadlines.Remove(RequiredFlag);

	if (AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		PlayerChar->RemoveFlag(RequiredFlag);
	}
}

void AWallWarpLink::OnInGameTimeChangedHandler(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute)
{
	CheckAutoClearDeadline();
}

void AWallWarpLink::OnPlayerFlagRemoved(FName FlagName)
{
	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	// 他の手段（会話のRemoveFlag等）で外れた場合に、期限の記録だけ残って次回の取得時に早く切れないようにする
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		GameInst->FlagAutoClearDeadlines.Remove(RequiredFlag);
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AWallWarpLink::UpdateFlagVisibility(const AMyProject1Character* PlayerChar)
{
	const bool bUnlocked = RequiredFlag.IsNone() || (PlayerChar && PlayerChar->HasFlag(RequiredFlag));

	SetActorHiddenInGame(!bUnlocked);
	SetActorEnableCollision(bUnlocked);
}

void AWallWarpLink::OnPlayerFlagAdded(FName FlagName)
{
	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (bAutoClearFlag)
	{
		// 新しくフラグが立った時点から数え直す（前回の期限記録が残っていても上書き）
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
		{
			GameInst->FlagAutoClearDeadlines.Remove(RequiredFlag);
		}
		StartAutoClearDeadline();
	}
}

void AWallWarpLink::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 徘徊中のNPCなどが衝突して誤発火しないよう、「Player」タグを持つアクターだけを対象にする
	if (!OtherActor->ActorHasTag(TEXT("Player"))) return;

	ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
	if (!PlayerChar) return;

	// IsReadyToInteract()（InteractToWarp時のターゲット対象判定）に使うため、
	// トリガー方式に関わらず「今ボックスに触れているか」は常に更新する
	bIsPlayerOverlapping = true;

	if (TriggerType != EWallWarpTriggerType::TouchToWarp) return;

	// ワープ直後の着地でこのボックスに再突入した場合は、猶予時間内なら無視して往復ワープを防ぐ
	if (const float* CooldownEndTime = RecentlyArrivedActors.Find(PlayerChar))
	{
		if (GetWorld()->GetTimeSeconds() < *CooldownEndTime)
		{
			return;
		}
		RecentlyArrivedActors.Remove(PlayerChar);
	}

	ExecuteWarp(PlayerChar);
}

void AWallWarpLink::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<ACharacter>(OtherActor))
	{
		bIsPlayerOverlapping = false;
	}
}

void AWallWarpLink::InteractWithLink(ACharacter* Interactor)
{
	if (TriggerType == EWallWarpTriggerType::InteractToWarp)
	{
		ExecuteWarp(Interactor);
	}
}

void AWallWarpLink::ExecuteWarp(ACharacter* TargetCharacter)
{
	if (!TargetCharacter || !TargetLink) return;

	// エリアChangeと同じ暗転（OnWarpFadeOutRequested）を挟む。GameInstance側で入力も禁止されるため、
	// 暗転中にプレイヤーが動けてしまうことはない。実際の移動は暗転後にExecuteWarpNowで行う
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		GameInst->RequestFadeThenWallWarp(this, TargetCharacter);
		return;
	}

	// GameInstanceが取得できない異常系のみ、暗転なしで即座にワープするフォールバック
	ExecuteWarpNow(TargetCharacter);
}

void AWallWarpLink::ExecuteWarpNow(ACharacter* TargetCharacter)
{
	if (!TargetCharacter || !TargetLink) return;

	// 相方リンクの少し前方（壁の外側）に出す。ExitOffsetDistanceが0だとTargetLinkの真上に出てしまい、
	// 相手側のボックスへ即座に再突入して往復してしまうため
	const FVector NewLocation = TargetLink->GetActorLocation() + TargetLink->GetActorForwardVector() * ExitOffsetDistance;
	const FRotator NewRotation = TargetLink->GetActorForwardVector().Rotation();

	// TeleportToはコンポーネントのUpdateOverlapsを介して、移動先ボックスのOnOverlapBeginを
	// 呼び出し元に戻る前に「同期的に」発火させる。クールダウンを先にセットしておかないと、
	// TargetLink側のOnOverlapBeginがノーガードでExecuteWarpを呼び返し、無限再帰（CTD）になる
	TargetLink->RecentlyArrivedActors.Add(TargetCharacter, GetWorld()->GetTimeSeconds() + TargetLink->ReTriggerCooldown);

	TargetCharacter->TeleportTo(NewLocation, NewRotation, false, true);
}
