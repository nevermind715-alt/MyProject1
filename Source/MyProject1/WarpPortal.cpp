#include "WarpPortal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "MyProject1GameInstance.h"
#include "MyProject1Character.h"
#include "Kismet/GameplayStatics.h"

#pragma execution_character_set("utf-8")

AWarpPortal::AWarpPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	CollisionBox->InitBoxExtent(FVector(100.f, 100.f, 100.f));

	CollisionBox->SetCollisionProfileName(TEXT("Trigger"));

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWarpPortal::OnOverlapBegin);
}

void AWarpPortal::BeginPlay()
{
	Super::BeginPlay();

	// フラグ制御を使わない（従来の）ポータルは何もしない
	if (RequiredFlag.IsNone()) return;

	if (bAutoClearFlag)
	{
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
		{
			GameInst->OnInGameTimeChanged.AddDynamic(this, &AWarpPortal::OnInGameTimeChangedHandler);
		}
	}

	InitFlagVisibilityWhenReady();
}

void AWarpPortal::InitFlagVisibilityWhenReady()
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));

	// プレイヤーがまだ生成されていない（レベル遷移直後・ローディング画面中など）。少し待って再試行
	if (!PlayerChar)
	{
		if (FlagInitAttempts++ < 240) // 0.25秒 * 240 = 最大60秒粘る
		{
			GetWorldTimerManager().SetTimer(FlagInitTimerHandle, this, &AWarpPortal::InitFlagVisibilityWhenReady, 0.25f, false);
		}
		return;
	}

	// フラグ通知の購読は一度だけ（同一マップでの「会話→フラグ付与」ライブ経路用）。
	// 別レベルからのUnlockedFlags一括復元はAddFlagを経由しないためOnFlagAddedは飛んでこない → 下のHasFlag直接判定で拾う
	if (!bFlagDelegatesBound)
	{
		PlayerChar->OnFlagAdded.AddDynamic(this, &AWarpPortal::OnPlayerFlagAdded);
		PlayerChar->OnFlagRemoved.AddDynamic(this, &AWarpPortal::OnPlayerFlagRemoved);
		bFlagDelegatesBound = true;
		FlagInitAttempts = 0; // ここからフラグ復元待ちの試行回数を数え直す
	}

	UpdateFlagVisibility(PlayerChar);

	if (PlayerChar->HasFlag(RequiredFlag))
	{
		// すでにフラグを持った状態で始まった場合（レベル移動後・ロード後）。期限の記録が無ければ今から数え始め、
		// 期限が過ぎていればその場で外す
		if (bAutoClearFlag)
		{
			StartAutoClearDeadline();
			CheckAutoClearDeadline();
		}
		return;
	}

	// プレイヤーは取れたが、フラグがまだ復元されていない可能性がある。別レベルからのステータス復元
	// (ApplyPendingCharacterLoad)がこのアクタの初期化より後にずれ込むケースを拾うため、数回だけ再チェックしてから
	// 諦める（以降はOnFlagAdded待ち）
	if (FlagInitAttempts++ < 20) // 0.25秒 * 20 = 5秒
	{
		GetWorldTimerManager().SetTimer(FlagInitTimerHandle, this, &AWarpPortal::InitFlagVisibilityWhenReady, 0.25f, false);
	}
}

void AWarpPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// フラグ初期化待ちのタイマーが回っている途中でアクタが消えた場合の後始末
	GetWorldTimerManager().ClearTimer(FlagInitTimerHandle);

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		GameInst->OnInGameTimeChanged.RemoveDynamic(this, &AWarpPortal::OnInGameTimeChangedHandler);
	}

	Super::EndPlay(EndPlayReason);
}

void AWarpPortal::StartAutoClearDeadline()
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst || GameInst->FlagAutoClearDeadlines.Contains(RequiredFlag)) return;

	const int32 NowMinutes = GameInst->TotalElapsedDays * 1440 + GameInst->CurrentTimeInMinutes;
	GameInst->FlagAutoClearDeadlines.Add(RequiredFlag, NowMinutes + FMath::RoundToInt(AutoClearFlagGameHours * 60.0f));
}

void AWarpPortal::CheckAutoClearDeadline()
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

void AWarpPortal::OnInGameTimeChangedHandler(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute)
{
	CheckAutoClearDeadline();
}

void AWarpPortal::OnPlayerFlagRemoved(FName FlagName)
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

void AWarpPortal::UpdateFlagVisibility(const AMyProject1Character* PlayerChar)
{
	const bool bUnlocked = RequiredFlag.IsNone() || (PlayerChar && PlayerChar->HasFlag(RequiredFlag));

	SetActorHiddenInGame(!bUnlocked);
	SetActorEnableCollision(bUnlocked);
}

void AWarpPortal::OnPlayerFlagAdded(FName FlagName)
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

void AWarpPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->ActorHasTag(TEXT("Player"))) return;

	if (ACharacter* PlayerChar = Cast<ACharacter>(OtherActor))
	{
		if (PortalType == EWarpPortalType::TouchToWarp)
		{
			ExecuteWarp(PlayerChar);
		}
	}
}

void AWarpPortal::InteractWithPortal(ACharacter* Interactor)
{
	if (PortalType == EWarpPortalType::InteractToWarp)
	{
		ExecuteWarp(Interactor);
	}
}

void AWarpPortal::ExecuteWarp(ACharacter* TargetCharacter)
{
	if (!TargetCharacter || TargetWarpID.IsNone()) return;

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInst->RequestWarp(TargetWarpID, TargetCharacter);
	}
}
