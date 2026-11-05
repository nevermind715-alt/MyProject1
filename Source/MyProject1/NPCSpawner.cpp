// NPCSpawner.cpp

#include "NPCSpawner.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EventDistributorComponent.h"
#include "QuestComponent.h"
#include "MyProject1GameInstance.h"

ANPCSpawner::ANPCSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	RootComponent = SpriteComponent;

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	ArrowComponent->SetupAttachment(RootComponent);
#endif
}

void ANPCSpawner::BeginPlay()
{
	Super::BeginPlay();

	// RequiredFlag運用：フラグの獲得／喪失に連動してスポーンする。開始時の自動スポーンは行わない。
	// レベル遷移をまたぐケース（別レベルで会話→フラグ付与、スポナーは別レベルに配置）にも対応するため、
	// プレイヤー生成とステータス復元を待ってからフラグを評価する
	if (!RequiredFlag.IsNone())
	{
		InitFlagSpawnWhenReady();
		return;
	}

	if (InitialSpawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ANPCSpawner::SpawnEnemy, InitialSpawnDelay, false);
	}
	else
	{
		SpawnEnemy();
	}
}

void ANPCSpawner::SpawnEnemy()
{
	// スポーン回数が上限に達している場合は何もしない（0は無制限）
	if (MaxSpawnCount > 0 && CurrentSpawnCount >= MaxSpawnCount)
	{
		return;
	}

	if (!EnemyClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FVector Location = GetActorLocation();
	FRotator Rotation = GetActorRotation();
	FTransform SpawnTransform(Rotation, Location);

	SpawnedEnemy = World->SpawnActorDeferred<AMyProject1Character>(
		EnemyClass,
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);

	if (SpawnedEnemy)
	{

		FDataTableRowHandle FinalJobRow = this->SpawnerJobRow;
		FString FinalNPCName = this->SpawnerNPCName;
		FVector FinalScale = FVector(1.0f, 1.0f, 1.0f);

		if (bEnableRareSpawn)
		{
			if (FMath::RandRange(0.0f, 100.0f) <= RareSpawnChance)
			{
				FinalJobRow = this->RareJobRow;
				FinalNPCName = this->RareNPCName;
				FinalScale = this->RareScale;
			}
		}

		SpawnedEnemy->JobRow = FinalJobRow;
		SpawnedEnemy->MyStats.NPCName = FinalNPCName;
		// レベル。FinishSpawning/ApplyJobDataより前に決めるため、HP・各ステータスはこのレベルから計算される。
		// プレイヤーが取れない場合（ゲーム開始直後の自動スポーン等）はSpawnerLevelのままにする
		int32 FinalLevel = this->SpawnerLevel;
		if (bScaleLevelToPlayer)
		{
			if (const AMyProject1Character* LevelPlayer = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				const int32 OffsetMin = FMath::Min(PlayerLevelOffsetMin, PlayerLevelOffsetMax);
				const int32 OffsetMax = FMath::Max(PlayerLevelOffsetMin, PlayerLevelOffsetMax);
				FinalLevel = FMath::Max(1, LevelPlayer->MyStats.Level + FMath::RandRange(OffsetMin, OffsetMax));
			}
		}

		SpawnedEnemy->MyStats.Level = FinalLevel;
		SpawnedEnemy->SetActorScale3D(FinalScale);

		// 遠い/小さく映る敵のアニメ更新・評価を間引く（URO）。AQuestNPCBaseと同じ理由
		SpawnedEnemy->GetMesh()->bEnableUpdateRateOptimizations = true;

		if (bOverridePatrolSettings)
		{
			SpawnedEnemy->PatrolRadius = this->SpawnerPatrolRadius;
			SpawnedEnemy->bCanPatrol = this->bSpawnerCanPatrol;
			SpawnedEnemy->bIsActiveEnemy = this->bSpawnerIsActiveEnemy;
			SpawnedEnemy->bCanLink = this->bSpawnerCanLink;

			SpawnedEnemy->AISightRadius = this->SpawnerSightRadius;
			SpawnedEnemy->AILoseSightRadius = this->SpawnerLoseSightRadius;
			SpawnedEnemy->AIVisionAngle = this->SpawnerVisionAngle;
			SpawnedEnemy->bAIEnableHearing = this->bSpawnerEnableHearing;
			SpawnedEnemy->AIHearingRange = this->SpawnerHearingRange;
		}

		SpawnedEnemy->OnDeathDelegate.AddDynamic(this, &ANPCSpawner::OnEnemyDeath);

		// ---------------------------------------------------------
		// ---------------------------------------------------------
		SpawnedEnemy->FinishSpawning(SpawnTransform);

		// ---------------------------------------------------------
		// ---------------------------------------------------------
		SpawnedEnemy->ApplyJobData();

		// 空欄でなければEventPoolIDを、bOverrideExtraParticipantMeshWithOwnMeshが有効なら敵自身のジョブメッシュを、
		// 空欄でなければWarpToRestartID（とEventDistributorChance）を、
		// この敵のEventDistributorComponent（設定されていれば）へ上書きする
		if (!SpawnerEventPoolID.IsNone() || bOverrideExtraParticipantMeshWithOwnMesh || !WarpToRestartID.IsNone() || bHideEnemyDuringAnimEvent || (DefeatDialogRow.DataTable && !DefeatDialogRow.RowName.IsNone()) || EventDistributorStatCondition.bUseStatCondition)
		{
			if (UEventDistributorComponent* EventComp = SpawnedEnemy->FindComponentByClass<UEventDistributorComponent>())
			{
				if (bHideEnemyDuringAnimEvent)
				{
					EventComp->bHideOwnerDuringAnimEventDefault = true;
				}

				if (!SpawnerEventPoolID.IsNone())
				{
					EventComp->EventPoolID = SpawnerEventPoolID;
				}

				if (bOverrideExtraParticipantMeshWithOwnMesh)
				{
					if (FJobAttributes* JobData = FinalJobRow.GetRow<FJobAttributes>(FinalJobRow.RowName.ToString()))
					{
						EventComp->ExtraParticipantMeshOverrideDefault = JobData->CharacterMesh;
					}
				}

				if (!WarpToRestartID.IsNone())
				{
					EventComp->WarpToRestartID = WarpToRestartID;
					EventComp->EventDistributorChance = EventDistributorChance;
				}

				// ステータス条件はWarpToRestartIDの有無に関係なく使われるため、条件オンの時だけ別に上書きする
				if (EventDistributorStatCondition.bUseStatCondition)
				{
					EventComp->EventDistributorStatCondition = EventDistributorStatCondition;
				}

				if (DefeatDialogRow.DataTable && !DefeatDialogRow.RowName.IsNone())
				{
					EventComp->DefeatDialogRow = DefeatDialogRow;
				}
			}
		}

		// 敗北時クリーンアップが有効なら、プレイヤーを倒した後の通知を購読する
		if (bRemoveOnPlayerDefeat || !FailQuestIDOnPlayerDefeat.IsNone() || ActorsToRemoveOnPlayerDefeat.Num() > 0 || !FlagToRemoveOnExit.IsNone())
		{
			if (UEventDistributorComponent* EventComp = SpawnedEnemy->FindComponentByClass<UEventDistributorComponent>())
			{
				EventComp->OnDefeatedPlayer.AddUObject(this, &ANPCSpawner::OnEnemyDefeatedPlayer);
			}
		}

		if (bOverrideBaseLoot)
		{
			SpawnedEnemy->PersonalLootTable = this->SpawnerLootTable;
		}
		else
		{
			SpawnedEnemy->PersonalLootTable.Append(this->SpawnerLootTable);
		}

		// =========================================================
		// =========================================================
		SpawnedEnemy->MyStats.MaxHP += SpawnerStatBonus.MaxHP;
		SpawnedEnemy->MyStats.BaseAttackPower += SpawnerStatBonus.AttackPower;
		SpawnedEnemy->MyStats.BaseDefensePower += SpawnerStatBonus.DefensePower;
		SpawnedEnemy->MyStats.STR += SpawnerStatBonus.STR;
		SpawnedEnemy->MyStats.VIT += SpawnerStatBonus.VIT;
		SpawnedEnemy->MyStats.DEX += SpawnerStatBonus.DEX;
		SpawnedEnemy->MyStats.AGI += SpawnerStatBonus.AGI;
		SpawnedEnemy->MyStats.Accuracy += SpawnerStatBonus.Accuracy;
		SpawnedEnemy->MyStats.Evasion += SpawnerStatBonus.Evasion;

		SpawnedEnemy->RecalculateFatigueAdjustedCombatStats();

		SpawnedEnemy->MyStats.HP = SpawnedEnemy->MyStats.MaxHP;

		// ポップ直後からプレイヤーを捕捉させる（BOSS用）。FinishSpawning済みのためAIコントローラーとBlackboardは
		// 構築済みで、SetCurrentTargetがBlackboardのTargetActorも更新し、ビヘイビアツリーが追跡・攻撃に入る
		if (bAggroPlayerOnSpawn)
		{
			if (AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				SpawnedEnemy->SetCurrentTarget(PlayerChar);
			}
		}

		// スポーン回数をカウント
		CurrentSpawnCount++;
	}
} 

bool ANPCSpawner::ShouldSpawnForFlagState(const AMyProject1Character* PlayerChar) const
{
	if (!PlayerChar || RequiredFlag.IsNone()) return false;
	if (!PlayerChar->HasFlag(RequiredFlag)) return false;

	// この周回で既に討伐済み（討伐フラグ所持）なら、レベル再入場でも重複POPさせない。
	// GrantFlagOnEnemyDeath未設定のスポナーではこの判定は無視される
	if (!GrantFlagOnEnemyDeath.IsNone() && PlayerChar->HasFlag(GrantFlagOnEnemyDeath))
	{
		return false;
	}
	return true;
}

void ANPCSpawner::InitFlagSpawnWhenReady()
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));

	// プレイヤーがまだ生成されていない（レベル遷移直後・ローディング画面中など）。少し待って再試行
	if (!PlayerChar)
	{
		if (FlagInitAttempts++ < 240) // 0.25秒 * 240 = 最大60秒粘る
		{
			GetWorldTimerManager().SetTimer(FlagInitTimerHandle, this, &ANPCSpawner::InitFlagSpawnWhenReady, 0.25f, false);
		}
		return;
	}

	// フラグ通知の購読は一度だけ（同一マップでの「会話→フラグ付与→POP」のライブ経路用）。
	// 別レベルからのUnlockedFlags一括復元はAddFlagを経由しないためOnFlagAddedは飛んでこない → 下のHasFlag直接判定で拾う
	if (!bFlagDelegatesBound)
	{
		PlayerChar->OnFlagAdded.AddDynamic(this, &ANPCSpawner::OnPlayerFlagAdded);
		PlayerChar->OnFlagRemoved.AddDynamic(this, &ANPCSpawner::OnPlayerFlagRemoved);
		bFlagDelegatesBound = true;
		FlagInitAttempts = 0; // ここからフラグ復元待ちの試行回数を数え直す
	}

	if (ShouldSpawnForFlagState(PlayerChar))
	{
		if (!bSpawnedForCurrentFlag && !SpawnedEnemy)
		{
			StartFlagSpawn();
		}
		return; // 確定。リトライ終了
	}

	// プレイヤーは取れたがRequiredFlagがまだ無い。別レベルからのステータス復元(ApplyPendingCharacterLoad)が
	// このスポナーの初期化より後にずれ込むケースを拾うため、数回だけ再チェックしてから諦める（以降はOnFlagAdded待ち）
	if (!PlayerChar->HasFlag(RequiredFlag) && FlagInitAttempts++ < 20) // 0.25秒 * 20 = 5秒
	{
		GetWorldTimerManager().SetTimer(FlagInitTimerHandle, this, &ANPCSpawner::InitFlagSpawnWhenReady, 0.25f, false);
	}
}

void ANPCSpawner::OnEnemyDeath(AActor* DeadActor)
{
	if (DeadActor == SpawnedEnemy)
	{
		SpawnedEnemy = nullptr;

		// 討伐フラグの付与（Delivery型お使いクエスト等、TargetIDで討伐を追えないクエストの報告ゲート用）
		if (!GrantFlagOnEnemyDeath.IsNone())
		{
			if (AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				PlayerChar->AddFlag(GrantFlagOnEnemyDeath);
			}
		}

		// 賞金の付与（闘技場など）。ワープ待ちの間にプレイヤーが倒れても勝った分は受け取れるよう、討伐の瞬間に渡す
		if (GilRewardOnEnemyDeath > 0)
		{
			if (AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				if (UInventoryComponent* InvComp = PlayerChar->FindComponentByClass<UInventoryComponent>())
				{
					InvComp->AddGil(GilRewardOnEnemyDeath);
					PlayerChar->OnReceiveLogMessage(FString::Printf(TEXT("賞金として %d￥ 手に入れた。"), GilRewardOnEnemyDeath), ELogMessageType::System);
				}
			}
		}

		// 初勝利フラグ（永続）。持っていれば何も起きないので、勝つたびに呼んでも付与されるのは最初の1回だけ
		if (!FirstVictoryFlag.IsNone())
		{
			if (AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				PlayerChar->AddFlag(FirstVictoryFlag);
			}
		}

		// 勝利時の退場（闘技場など）。VictoryWarpDelaySeconds秒の余韻を置いてからワープする
		if (!VictoryWarpID.IsNone())
		{
			if (VictoryWarpDelaySeconds > 0.0f)
			{
				GetWorldTimerManager().SetTimer(VictoryWarpTimerHandle, this, &ANPCSpawner::ExecuteVictoryWarp, VictoryWarpDelaySeconds, false);
			}
			else
			{
				ExecuteVictoryWarp();
			}
		}

		// RequiredFlag運用時は「1周1体」。フラグが外れて再度立つまでは自動リスポーンしない
		if (!RequiredFlag.IsNone())
		{
			return;
		}

		// スポーン回数の上限に達していなければリスポーンする（0は無制限）
		if (MaxSpawnCount <= 0 || CurrentSpawnCount < MaxSpawnCount)
		{
			GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ANPCSpawner::SpawnEnemy, RespawnTime, false);
		}
	}
}

void ANPCSpawner::ExecuteVictoryWarp()
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!PlayerChar || !GameInst) return;

	// 余韻の待機中に相打ち等でプレイヤーが倒れていたら、敗北側の処理（HandleDefeatWithoutEvent等）に任せる。
	// ここでワープすると、敗北側のワープ・復帰と二重に走ってしまう
	if (PlayerChar->IsDead()) return;

	// RequestFadeThenRemoveFlagは、先に予約されたWarpの暗転に相乗りする仕様のため、必ずRequestWarpの後に呼ぶ
	// （消去はExecuteWarpProcessの先頭＝画面が真っ暗な間、かつ別レベルへ移動する前に反映される）
	GameInst->RequestWarp(VictoryWarpID, PlayerChar);

	// RequiredFlagを残したままだと、別レベルから戻った時に再びスポーンしてしまう（敗北側OnEnemyDefeatedPlayerと同じ理由）。
	// 外すとOnPlayerFlagRemovedが呼ばれ、次の周回に向けた再武装と、RequiredFlag連動の表示切替の復帰が行われる
	// RequestFadeThenRemoveFlagの予約枠は1つだけなので、FlagToRemoveOnExitがある時は、見た目に影響する側
	// （見せ用NPCの再表示）を暗転に相乗りさせ、RequiredFlagは暗転なしで先に外す（外してもスポナーの再武装等だけで見た目は変わらない）
	if (!FlagToRemoveOnExit.IsNone())
	{
		if (!RequiredFlag.IsNone())
		{
			PlayerChar->RemoveFlag(RequiredFlag);
		}
		GameInst->RequestFadeThenRemoveFlag(FlagToRemoveOnExit, PlayerChar);
	}
	else if (!RequiredFlag.IsNone())
	{
		GameInst->RequestFadeThenRemoveFlag(RequiredFlag, PlayerChar);
	}
}

void ANPCSpawner::OnEnemyDefeatedPlayer()
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (PlayerChar)
	{
		if (!FailQuestIDOnPlayerDefeat.IsNone())
		{
			if (UQuestComponent* QuestComp = PlayerChar->FindComponentByClass<UQuestComponent>())
			{
				// FailQuest自身はObjectiveClearedFlagしか外さない。受注で立てたAcceptFlagも外すことで、
				// それをRequiredFlagにしているQuestItemPoint等が既存の仕組み（OnFlagRemoved）で自動的に非表示になり、
				// セーブ／レベル遷移をまたいでも復活しない
				FQuestData QuestData;
				const bool bHasData = QuestComp->GetQuestData(FailQuestIDOnPlayerDefeat, QuestData);

				QuestComp->FailQuest(FailQuestIDOnPlayerDefeat);

				if (bHasData && !QuestData.AcceptFlag.IsNone())
				{
					PlayerChar->RemoveFlag(QuestData.AcceptFlag);
				}
			}
		}

		// フラグ運用の場合、フラグを残したままだと別レベルから戻った時にこのスポナーが再びスポーンしてしまう。
		// ここで外すとOnPlayerFlagRemovedが呼ばれ、次の周回に向けた再武装・討伐フラグの解除も行われる
		if (!RequiredFlag.IsNone())
		{
			PlayerChar->RemoveFlag(RequiredFlag);
		}

		// 複数スポナーで共有している見せ用NPC等のフラグ（勝利時の退場と同じく、ここで外して元の表示へ戻す）
		if (!FlagToRemoveOnExit.IsNone())
		{
			PlayerChar->RemoveFlag(FlagToRemoveOnExit);
		}
	}

	for (AActor* Target : ActorsToRemoveOnPlayerDefeat)
	{
		if (IsValid(Target))
		{
			Target->Destroy();
		}
	}

	if (bRemoveOnPlayerDefeat)
	{
		RemoveEnemyAfterEvent();
	}
}

void ANPCSpawner::RemoveEnemyAfterEvent()
{
	if (!IsValid(SpawnedEnemy)) return;

	// イベント進行中は、この敵がEventContextActor（AnimEventのPlayTarget=NPC・非表示対象）として
	// 暗転・ワープ後にも参照される（UMyProject1GameInstance::BeginAnimEventSequenceIfNeeded）。
	// ここで消すとイベントが正常に再生されなくなるため、イベントが終わるまで待ってから削除する
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		if (GameInst->bHasActiveEvent)
		{
			GetWorldTimerManager().SetTimer(RemoveEnemyTimerHandle, this, &ANPCSpawner::RemoveEnemyAfterEvent, 0.5f, false);
			return;
		}
	}

	// Destroyは討伐ではないためOnEnemyDeath（討伐フラグ付与・自動リスポーン）は呼ばれない
	SpawnedEnemy->Destroy();
	SpawnedEnemy = nullptr;
}

void ANPCSpawner::OnPlayerFlagAdded(FName FlagName)
{
	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	// この周回で既にスポーン済み、または個体がまだ生存中なら何もしない
	if (bSpawnedForCurrentFlag || SpawnedEnemy) return;

	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!ShouldSpawnForFlagState(PlayerChar)) return;

	StartFlagSpawn();
}

void ANPCSpawner::StartFlagSpawn()
{
	if (FlagSpawnDelaySeconds > 0.0f)
	{
		// 待機中にOnPlayerFlagAddedやInitFlagSpawnWhenReadyが再度呼ばれても二重にスポーンしないよう、先に印を付ける
		bSpawnedForCurrentFlag = true;
		GetWorldTimerManager().SetTimer(FlagSpawnTimerHandle, this, &ANPCSpawner::SpawnEnemy, FlagSpawnDelaySeconds, false);
		return;
	}

	SpawnEnemy();
	bSpawnedForCurrentFlag = true;
}

void ANPCSpawner::OnPlayerFlagRemoved(FName FlagName)
{
	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	// 次にフラグが立った時、再び1体スポーンできるようにする
	bSpawnedForCurrentFlag = false;

	// スポーン待機中（FlagSpawnDelaySeconds）にフラグが外れたら、湧かせずにキャンセルする
	GetWorldTimerManager().ClearTimer(FlagSpawnTimerHandle);

	// 周回リセット：前周に立てた討伐フラグも一緒に外し、依頼主への報告ゲートを再ロックする
	if (!GrantFlagOnEnemyDeath.IsNone())
	{
		if (AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			PlayerChar->RemoveFlag(GrantFlagOnEnemyDeath);
		}
	}
}
