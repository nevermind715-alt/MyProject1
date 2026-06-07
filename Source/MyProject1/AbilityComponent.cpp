#include "AbilityComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RpgCharacterInterface.h" 
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "RpgDamageCalculator.h"
#include "MyProject1Character.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsCasting = false;
	CurrentTarget = nullptr;
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (RecastTimers.Num() > 0)
	{
		TArray<FName> KeysToRemove;
		for (auto& Pair : RecastTimers)
		{
			Pair.Value -= DeltaTime;
			if (Pair.Value <= 0.0f)
			{
				KeysToRemove.Add(Pair.Key);
			}
		}

		for (FName Key : KeysToRemove)
		{
			RecastTimers.Remove(Key);
		}
	}
}

void UAbilityComponent::LearnAbility(FName AbilityID)
{
	if (!LearnedAbilities.Contains(AbilityID))
	{
		LearnedAbilities.Add(AbilityID);
	}
}

bool UAbilityComponent::TryCastAbility(FName AbilityID, AActor* TargetActor)
{
	if (bIsCasting) return false;
	if (!AbilityDataTable) return false;

	FAbilityData* Data = AbilityDataTable->FindRow<FAbilityData>(AbilityID, TEXT("AbilityCast"));
	if (!Data) return false;

	if (RecastTimers.Contains(AbilityID))
	{
		return false;
	}

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (!RpgInterface) return false;

	FCharacterStats& Stats = RpgInterface->GetCharacterStats();

	// =========================================================
	// ★追加：対象（ターゲット）の自動決定と有効性チェック
	// =========================================================
	AActor* FinalTarget = TargetActor;

	// 引数の TargetActor が空の場合、オーナー（キャラクター）が現在ロックオンしているターゲットを適用
	if (!FinalTarget)
	{
		if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner()))
		{
			FinalTarget = OwnerChar->CurrentTarget; // キャラクターが保持しているCurrentTargetを取得
		}
	}

	// 敵単体対象（EnemySingle）のアビリティの場合のチェック
	if (Data->TargetType == EAbilityTargetType::EnemySingle)
	{
		// ターゲットがいない、またはターゲットが死んでいる場合は発動させない
		if (!FinalTarget)
		{
			RpgInterface->OnReceiveLogMessage(TEXT("有効なターゲットがいません。"), ELogMessageType::System);
			return false;
		}

		AMyProject1Character* TargetChar = Cast<AMyProject1Character>(FinalTarget);
		if (TargetChar && TargetChar->IsDead())
		{
			RpgInterface->OnReceiveLogMessage(TEXT("その相手はすでに倒されています。"), ELogMessageType::System);
			return false;
		}
	}

	// =========================================================
	// スタミナの消費チェックと実行
	// =========================================================
	if (Data->CostStamina > 0.0f)
	{
		if (Stats.Stamina < Data->CostStamina)
		{
			RpgInterface->OnReceiveLogMessage(TEXT("スタミナが足りない！"), ELogMessageType::System);
			return false;
		}

		Stats.Stamina -= Data->CostStamina;

		if (AMyProject1Character* MyChar = Cast<AMyProject1Character>(GetOwner()))
		{
			if (MyChar->OnStaminaChangedDelegate.IsBound())
			{
				MyChar->OnStaminaChangedDelegate.Broadcast(Stats.Stamina, Stats.MaxStamina);
			}
		}

		RpgInterface->NotifyStatsChanged();
	}

	// --- 詠唱の開始 ---
	bIsCasting = true;
	CurrentCastingAbilityID = AbilityID;
	CurrentTarget = FinalTarget; // 自動決定した確定ターゲットを保存

	FString LogMsg = FString::Printf(TEXT("%s は %s の構え。"), *Stats.NPCName, *Data->AbilityName);
	RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);

	if (Data->CastTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(CastTimerHandle, this, &UAbilityComponent::ExecuteAbility, Data->CastTime, false);
	}
	else
	{
		ExecuteAbility();
	}

	return true;
}

void UAbilityComponent::CancelCasting()
{
	if (bIsCasting)
	{
		GetWorld()->GetTimerManager().ClearTimer(CastTimerHandle);
		bIsCasting = false;

		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
		{
			RpgInterface->OnReceiveLogMessage(TEXT("動作が中断された！"), ELogMessageType::System);
		}
	}
}

void UAbilityComponent::ExecuteAbility()
{
	bIsCasting = false;

	if (!AbilityDataTable) return;
	FAbilityData* Data = AbilityDataTable->FindRow<FAbilityData>(CurrentCastingAbilityID, TEXT("AbilityExecute"));
	if (!Data) return;

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (RpgInterface)
	{
		FString LogMsg = FString::Printf(TEXT("%s が発動！"), *Data->AbilityName);
		RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);

		// --- 1. アニメーション（発動モーション）の再生 ---
		if (Data->ExecuteMontage)
		{
			if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
			{
				OwnerChar->PlayAnimMontage(Data->ExecuteMontage);
			}
		}

		// =========================================================
		// --- 2. 効果の適用 A：攻撃アビリティ（ダメージ技）の場合 ---
		// =========================================================
		if (Data->bIsAttackAbility)
		{
			AActor* AttackTarget = nullptr;
			if (Data->TargetType == EAbilityTargetType::EnemySingle) AttackTarget = CurrentTarget;
			else if (Data->TargetType == EAbilityTargetType::Self) AttackTarget = GetOwner();
			else AttackTarget = CurrentTarget ? CurrentTarget : GetOwner();

			if (AttackTarget)
			{
				IRpgCharacterInterface* TargetInterface = Cast<IRpgCharacterInterface>(AttackTarget);
				if (TargetInterface)
				{
					AMyProject1Character* TargetChar = Cast<AMyProject1Character>(AttackTarget);

					if (!TargetChar || !TargetChar->IsDead())
					{
						FCharacterStats& TargetStats = TargetInterface->GetCharacterStats();
						FString TargetName = TargetStats.NPCName;
						FCharacterStats& CasterStats = RpgInterface->GetCharacterStats();

						// --- ダメージとヒット数、クリティカル有無を合算・記録する変数を用意 ---
						float TotalDamage = 0.0f;
						int32 ActualHitCount = 0;
						bool bAnyCritical = false;

						// ヒット数分ループして内部計算のみ行う
						for (int32 i = 0; i < Data->HitCount; ++i)
						{
							if (TargetChar && TargetChar->IsDead()) break;

							// RpgDamageCalculatorに丸投げする！
							// 引数：(攻撃者, 防御者, 技の倍率(1.0), クリティカルボーナス, 基本D値)
							FDamageResult Result = URpgDamageCalculator::CalculateDamage(
								CasterStats,
								TargetStats,
								1.0f,
								Data->CriticalRate,
								Data->BaseDamage
							);

							// 命中した場合
							if (Result.bIsHit)
							{
								TotalDamage += Result.DamageAmount; // ダメージを合算
								ActualHitCount++;                   // ヒット回数を加算

								if (Result.bIsCritical)
								{
									bAnyCritical = true;            // 1回でもクリティカルが出たらフラグを立てる
								}
							}
						}

						// --- ループが終わった後に、合算した結果を元にログを出力・ダメージを適用 ---
						if (ActualHitCount > 0)
						{
							// ダメージを敵アクターに適用（合計ダメージを1回で与える）
							UGameplayStatics::ApplyDamage(
								AttackTarget,
								TotalDamage,
								GetOwner()->GetInstigatorController(),
								GetOwner(),
								bAnyCritical ? UCriticalDamageType::StaticClass() : UDamageType::StaticClass()
							);

							FString DmgLogMsg;

							// もし多段ヒットの設定（HitCount > 1）なら、ログにヒット数も記載する（例：3Hit!）
							if (Data->HitCount > 1)
							{
								if (bAnyCritical)
								{
									DmgLogMsg = FString::Printf(TEXT("【%s】(%d Hit!) → %sにクリティカル！ 計 %.0f のダメージ！"), *Data->AbilityName, ActualHitCount, *TargetName, TotalDamage);
								}
								else
								{
									DmgLogMsg = FString::Printf(TEXT("【%s】(%d Hit!) → %sに 計 %.0f のダメージ！"), *Data->AbilityName, ActualHitCount, *TargetName, TotalDamage);
								}
							}
							else
							{
								// 単発技の場合
								if (bAnyCritical)
								{
									DmgLogMsg = FString::Printf(TEXT("【%s】→ %sにクリティカル！ %.0f のダメージ！"), *Data->AbilityName, *TargetName, TotalDamage);
								}
								else
								{
									DmgLogMsg = FString::Printf(TEXT("【%s】→ %sに %.0f のダメージ！"), *Data->AbilityName, *TargetName, TotalDamage);
								}
							}

							RpgInterface->OnReceiveLogMessage(DmgLogMsg, bAnyCritical ? ELogMessageType::Critical : ELogMessageType::DamageDealt);
						}
						else
						{
							// 1回も命中しなかった（全弾ミス）場合
							FString MissLogMsg = FString::Printf(TEXT("【%s】→ %sにミス！"), *Data->AbilityName, *TargetName);
							RpgInterface->OnReceiveLogMessage(MissLogMsg, ELogMessageType::Default);
						}
					}
				}
			}
		}

		// =========================================================
		// --- 2. 効果の適用 B：バフ・デバフ・回復（Effects配列） ---
		// =========================================================
		for (const FItemEffect& Effect : Data->Effects)
		{
			// ※ 新しい攻撃システムを使う場合、Effects配列に混ざっている旧式の「HPマイナス効果」は無視する
			if (Data->bIsAttackAbility && Effect.TargetStat == ETargetStat::HP && Effect.EffectAmount < 0.0f)
			{
				continue;
			}

			AActor* EffectTarget = nullptr;
			if (Data->TargetType == EAbilityTargetType::EnemySingle) EffectTarget = CurrentTarget;
			else if (Data->TargetType == EAbilityTargetType::Self) EffectTarget = GetOwner();
			else EffectTarget = CurrentTarget ? CurrentTarget : GetOwner();

			if (!EffectTarget) continue;

			IRpgCharacterInterface* TargetInterface = Cast<IRpgCharacterInterface>(EffectTarget);
			if (!TargetInterface) continue;

			FCharacterStats& TargetStats = TargetInterface->GetCharacterStats();
			FString TargetName = TargetStats.NPCName;

			// 旧式のダメージ処理（bIsAttackAbilityのチェックを入れていない魔法用）
			if (Effect.TargetStat == ETargetStat::HP && Effect.EffectAmount < 0.0f)
			{
				AMyProject1Character* TargetChar = Cast<AMyProject1Character>(EffectTarget);
				if (TargetChar && TargetChar->IsDead()) continue;

				float BaseDamage = FMath::Abs(Effect.EffectAmount);
				FCharacterStats& CasterStats = RpgInterface->GetCharacterStats();
				float FinalDamage = BaseDamage + (CasterStats.AttackPower * 1.5f);

				UGameplayStatics::ApplyDamage(
					EffectTarget,
					FinalDamage,
					GetOwner()->GetInstigatorController(),
					GetOwner(),
					UDamageType::StaticClass()
				);

				FString DmgLogMsg = FString::Printf(TEXT("【%s】→ %sに %.0f のダメージ！"), *Data->AbilityName, *TargetName, FinalDamage);
				RpgInterface->OnReceiveLogMessage(DmgLogMsg, ELogMessageType::DamageDealt);
			}
			// プラス効果（回復やバフ・デバフの適用）の場合
			else if (Effect.EffectAmount > 0.0f)
			{
				switch (Effect.TargetStat)
				{
				case ETargetStat::HP:
					TargetStats.HP = FMath::Min(TargetStats.HP + Effect.EffectAmount, TargetStats.MaxHP);
					RpgInterface->OnReceiveLogMessage(FString::Printf(TEXT("%s のHPが %.0f 回復！"), *TargetName, Effect.EffectAmount), ELogMessageType::Default);

					if (AMyProject1Character* MyChar = Cast<AMyProject1Character>(EffectTarget))
					{
						MyChar->OnHPChanged(TargetStats.HP, TargetStats.MaxHP);
						if (MyChar->OnHPChangedDelegate.IsBound())
						{
							MyChar->OnHPChangedDelegate.Broadcast(TargetStats.HP, TargetStats.MaxHP);
						}
					}
					break;

				case ETargetStat::Stamina:
					TargetStats.Stamina = FMath::Min(TargetStats.Stamina + Effect.EffectAmount, TargetStats.MaxStamina);
					RpgInterface->OnReceiveLogMessage(FString::Printf(TEXT("%s のスタミナが %.0f 回復！"), *TargetName, Effect.EffectAmount), ELogMessageType::Default);

					if (AMyProject1Character* MyChar = Cast<AMyProject1Character>(EffectTarget))
					{
						if (MyChar->OnStaminaChangedDelegate.IsBound())
						{
							MyChar->OnStaminaChangedDelegate.Broadcast(TargetStats.Stamina, TargetStats.MaxStamina);
						}
					}
					break;

				default:
					break;
				}

				TargetInterface->NotifyStatsChanged();
			}
		}
	}

	// --- 3. リキャスト（クールダウン）を開始 ---
	if (Data->RecastTime > 0.0f)
	{
		RecastTimers.Add(CurrentCastingAbilityID, Data->RecastTime);
	}
}