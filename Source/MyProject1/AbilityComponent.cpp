#include "AbilityComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RpgCharacterInterface.h" 
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "MyProject1Character.h"

UAbilityComponent::UAbilityComponent()
{
	// リキャスト（クールダウン）タイマーを毎フレーム減らすためにTickを有効にします
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

	// --- リキャスト（クールダウン）のタイマーを減らす処理 ---
	if (RecastTimers.Num() > 0)
	{
		TArray<FName> KeysToRemove;
		for (auto& Pair : RecastTimers)
		{
			Pair.Value -= DeltaTime; // 経過時間を引く
			if (Pair.Value <= 0.0f)
			{
				KeysToRemove.Add(Pair.Key); // 0秒以下になったら削除リストへ
			}
		}

		// 完了したリキャストをリストから消去（再び使えるようになる）
		for (FName Key : KeysToRemove)
		{
			RecastTimers.Remove(Key);
		}
	}
}

void UAbilityComponent::LearnAbility(FName AbilityID)
{
	// まだ覚えていなければリストに追加
	if (!LearnedAbilities.Contains(AbilityID))
	{
		LearnedAbilities.Add(AbilityID);
	}
}

bool UAbilityComponent::TryCastAbility(FName AbilityID, AActor* TargetActor)
{
	// 既に何かを詠唱中なら弾く
	if (bIsCasting) return false;
	if (!AbilityDataTable) return false;

	// データテーブルからアビリティの設計図を取得
	FAbilityData* Data = AbilityDataTable->FindRow<FAbilityData>(AbilityID, TEXT("AbilityCast"));
	if (!Data) return false;

	// リキャスト（クールダウン）中なら弾く
	if (RecastTimers.Contains(AbilityID))
	{
		return false;
	}

	// オーナー（主人公や敵）のインターフェース窓口を取得
	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner());
	if (!RpgInterface) return false;

	FCharacterStats& Stats = RpgInterface->GetCharacterStats();

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

		// スタミナを消費する
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
	CurrentTarget = TargetActor;

	// ログの送信（「○○は ケアル を唱えた。」など）
	FString LogMsg = FString::Printf(TEXT("%s は %s を唱えた。"), *Stats.NPCName, *Data->AbilityName);
	RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);

	if (Data->CastTime > 0.0f)
	{
		// 詠唱時間がある場合はタイマーをセットして待つ
		GetWorld()->GetTimerManager().SetTimer(CastTimerHandle, this, &UAbilityComponent::ExecuteAbility, Data->CastTime, false);
	}
	else
	{
		// 詠唱時間が0（ウェポンスキルなど）なら即時発動
		ExecuteAbility();
	}

	return true;
}

void UAbilityComponent::CancelCasting()
{
	if (bIsCasting)
	{
		// 詠唱タイマーを強制ストップ
		GetWorld()->GetTimerManager().ClearTimer(CastTimerHandle);
		bIsCasting = false;

		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(GetOwner()))
		{
			RpgInterface->OnReceiveLogMessage(TEXT("詠唱が中断された！"), ELogMessageType::System);
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
		// ログの送信
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

		// --- 2. 効果（ダメージ・回復など）の適用 ---
		for (const FItemEffect& Effect : Data->Effects)
		{
			// 効果の対象を決定（ターゲットがいなければ自分自身を対象にする）
			AActor* EffectTarget = CurrentTarget ? CurrentTarget : GetOwner();
			IRpgCharacterInterface* TargetInterface = Cast<IRpgCharacterInterface>(EffectTarget);

			if (!TargetInterface) continue;

			FCharacterStats& TargetStats = TargetInterface->GetCharacterStats();
			FString TargetName = TargetStats.NPCName;

			// ▼ A. マイナス効果（ダメージ攻撃）の場合
			if (Effect.TargetStat == ETargetStat::HP && Effect.EffectAmount < 0.0f)
			{
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

				FString DmgLogMsg = FString::Printf(TEXT("%s に %.0f のダメージ！"), *TargetName, FinalDamage);
				RpgInterface->OnReceiveLogMessage(DmgLogMsg, ELogMessageType::DamageDealt);
			}
			// ▼ B. プラス効果（回復やバフなど）の場合
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

				// UIにステータス変化を通知
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