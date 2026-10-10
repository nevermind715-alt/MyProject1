// NPCSpawner.cpp

#include "NPCSpawner.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
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
		SpawnedEnemy->MyStats.Level = this->SpawnerLevel;
		SpawnedEnemy->SetActorScale3D(FinalScale);

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
		if (!SpawnerEventPoolID.IsNone() || bOverrideExtraParticipantMeshWithOwnMesh || !WarpToRestartID.IsNone() || bHideEnemyDuringAnimEvent)
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
			}
		}

		// 敗北時クリーンアップが有効なら、プレイヤーを倒した後の通知を購読する
		if (bRemoveOnPlayerDefeat || !FailQuestIDOnPlayerDefeat.IsNone() || ActorsToRemoveOnPlayerDefeat.Num() > 0)
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
			SpawnEnemy();
			bSpawnedForCurrentFlag = true;
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

	SpawnEnemy();
	bSpawnedForCurrentFlag = true;
}

void ANPCSpawner::OnPlayerFlagRemoved(FName FlagName)
{
	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	// 次にフラグが立った時、再び1体スポーンできるようにする
	bSpawnedForCurrentFlag = false;

	// 周回リセット：前周に立てた討伐フラグも一緒に外し、依頼主への報告ゲートを再ロックする
	if (!GrantFlagOnEnemyDeath.IsNone())
	{
		if (AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			PlayerChar->RemoveFlag(GrantFlagOnEnemyDeath);
		}
	}
}
