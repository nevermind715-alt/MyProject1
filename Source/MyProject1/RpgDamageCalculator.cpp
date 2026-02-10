#include "RpgDamageCalculator.h"

FDamageResult URpgDamageCalculator::CalculateDamage(const FCharacterStats& AttackerStats, const FCharacterStats& DefenderStats)
{
	FDamageResult Result;
	float LevelDiff = (float)DefenderStats.Level - (float)AttackerStats.Level;

	// --- 1. 命中判定 (Accuracy vs Evasion) ---
	// 基本命中率を 75% とし、命中と回避の差分をそのまま加算（FF11風の計算）
	// 例：命中102 vs 回避100 なら 77% になる
	float HitChance = 75.0f + (AttackerStats.Accuracy - DefenderStats.Evasion);

	float DamageMultiplier = 1.0f;

	// --- 三段階の絶望モデル（レベル差補正） ---
	// ここは既存のロジックを維持し、レベル差がある場合は命中率をさらに削ります
	if (LevelDiff > 0)
	{
		if (LevelDiff <= 3.0f)
		{
			HitChance -= LevelDiff * 8.0f;
		}
		else if (LevelDiff <= 6.0f)
		{
			HitChance = 66.0f - ((LevelDiff - 3.0f) * 12.0f);
			DamageMultiplier = 1.0f - ((LevelDiff - 3.0f) * 0.2f);
		}
		else
		{
			// 7レベル差以上：絶望
			HitChance = 20.0f; // 命中キャップをさらに厳しく
			DamageMultiplier = 0.1f;
		}
	}

	// 命中率を 5% ～ 95% の間に制限（どんなに命中が高くても 5% はミスする）
	HitChance = FMath::Clamp(HitChance, 5.0f, 95.0f);

	if (FMath::RandRange(0.0f, 100.0f) > HitChance)
	{
		Result.bIsHit = false;
		return Result;
	}
	Result.bIsHit = true;

	// --- 2. ダメージ計算 ---
	// 攻撃力と防御力の基本計算
	float BaseDamage = (AttackerStats.AttackPower - (DefenderStats.DefensePower / 2.0f)) * DamageMultiplier;
	if (BaseDamage < 1.0f) BaseDamage = 1.0f;

	// --- 3. クリティカル判定 ---
	// DEXとAGIの差も隠し味として残すと「ステータスの意味」がより深まります
	float FinalCritChance = 5.0f + (AttackerStats.DEX - DefenderStats.AGI) * 0.2f;

	// レベル差によるクリティカル補正
	if (LevelDiff > 0)
	{
		FinalCritChance -= LevelDiff * 2.0f;
	}
	else
	{
		FinalCritChance += FMath::Abs(LevelDiff) * 2.0f;
	}

	FinalCritChance = FMath::Clamp(FinalCritChance, 1.0f, 35.0f);

	if (FMath::RandRange(0.0f, 100.0f) < FinalCritChance)
	{
		Result.bIsCritical = true;
		Result.DamageAmount = FMath::FloorToFloat(BaseDamage * 1.5f); // クリティカルは1.5倍
	}
	else
	{
		Result.bIsCritical = false;
		// 通常ダメージのバラつき
		Result.DamageAmount = FMath::FloorToFloat(BaseDamage * FMath::RandRange(0.9f, 1.1f));
	}

	return Result;
}