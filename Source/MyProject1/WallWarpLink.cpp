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
