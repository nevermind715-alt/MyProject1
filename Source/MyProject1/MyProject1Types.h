#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // generatedより上に書く
#include "MyProject1Types.generated.h" // これが必ず最後

// --- 追加：武器タイプの定義 ---
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	HandToHand  UMETA(DisplayName = "格闘"),
	Sword       UMETA(DisplayName = "片手剣"),
	GreatSword  UMETA(DisplayName = "両手剣"),
	Axe         UMETA(DisplayName = "斧")
};

// MyProject1Types.h に追加、もしくは Character.h の UCLASS 前に追加
UENUM(BlueprintType)
enum class ELogMessageType : uint8
{
	Default      UMETA(DisplayName = "Default"),      // 白色（通常の行動など）
	DamageDealt  UMETA(DisplayName = "DamageDealt"),  // 自分が与えたダメージ
	DamageTaken  UMETA(DisplayName = "DamageTaken"),  // 自分が受けたダメージ
	ExpGain      UMETA(DisplayName = "ExpGain"),      // 経験値取得（青系）
	System       UMETA(DisplayName = "System"),       // システム（黄色や緑系）
	Critical     UMETA(DisplayName = "Critical"),      // クリティカル（強調）
	Dialogue     UMETA(DisplayName = "Dialogue")
};

// --- 追加：ジョブの基本データ（データテーブル用） ---
USTRUCT(BlueprintType)
struct FJobAttributes : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	float BaseHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	float BaseSTR = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	EWeaponType DefaultWeapon = EWeaponType::HandToHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<USoundBase> AttackSound;

	// そのジョブが装備する武器のメッシュ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;

	// ジョブごとの攻撃モーションセット（必要に応じて）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TArray<UAnimMontage*> AttackMontages;

	/** ★追加：クリティカルヒット時の音 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<USoundBase> CriticalSound;
};

// ... FCharacterStats などの既存コード

// --- ステータス管理用の構造体 ---
USTRUCT(BlueprintType)
struct FCharacterStats
{
	GENERATED_BODY()

	// --- 追加：ログに表示するための名前 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FString NPCName = TEXT("Monster");

	// --- 既存のステータス ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float HP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHP = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float STR = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float VIT = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float DEX = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AGI = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float AttackPower = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float DefensePower = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Stamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Accuracy = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Evasion = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Luck = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Fame = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Favor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Hostility = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Charm = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Mental = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Energy = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxEnergy = 100.0f; // 疲労度の限界値

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float BaseEnergy = 0.0f; // 蓄積疲労度（休息してもこれ以上は回復しない値）

	// --- 予備・カスタムステータス枠 ---
	// 好きな名前（FName）と数値（float）を自由にペアにして追加できるリスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Extra")
	TMap<FName, float> ExtraStats;


	// --- レベル・経験値の追加 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 CurrentXP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MaxXP = 500; // 次のレベルまでに必要な経験値

	// --- 追加：レベルアップ判定と報酬 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bCanLevelUp = false; // プレイヤーのみTrueにする

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 ExperienceReward = 50; // 敵が持っている経験値
};

// --- 計算結果を返す用の構造体（変更なし） ---
USTRUCT(BlueprintType)
struct FDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float DamageAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsHit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsCritical = false;
};

// アイテムの種類
UENUM(BlueprintType)
enum class EItemType : uint8
{
	Consumable  UMETA(DisplayName = "消費アイテム"),
	Equipment   UMETA(DisplayName = "装備品"),
	KeyItem     UMETA(DisplayName = "だいじなもの"),
	Material    UMETA(DisplayName = "素材")
};

// ★追加：どのステータスを変化させるかの定義
UENUM(BlueprintType)
enum class ETargetStat : uint8
{
	None        UMETA(DisplayName = "なし"),
	HP          UMETA(DisplayName = "HP"),
	MP          UMETA(DisplayName = "MP"),
	STR         UMETA(DisplayName = "STR (腕力)"),
	DEX         UMETA(DisplayName = "DEX (器用さ)"),
	VIT         UMETA(DisplayName = "VIT (生命力)"),
	AGI         UMETA(DisplayName = "AGI (素早さ)"),
	Stamina     UMETA(DisplayName = "Stamina (持久力)"),
	Accuracy    UMETA(DisplayName = "命中率"),
	Evasion     UMETA(DisplayName = "回避率"),
	AttackPower UMETA(DisplayName = "攻撃力")
};


USTRUCT(BlueprintType)
struct FItemEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETargetStat TargetStat = ETargetStat::None;

	/** 効果量（回復なら正、ダメージなら負） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectAmount = 0.0f;

	/** 効果時間（0なら即時、0より大きければリジェネやバフとして扱う想定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectDuration = 0.0f;
};


// アイテムの基本データ（データテーブルの1行分）
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY() // これが構造体の先頭に必要です

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Price = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SellPrice = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsRare = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEx = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Effect")
	TArray<FItemEffect> Effects;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	bool bConsumeOnUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Audio")
	TSoftObjectPtr<USoundBase> UseSound;
	
};

// 3. インベントリ用：カバンの中身（スロット）
USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	// どのアイテムか？（データテーブルの行名）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemID;

	// 何個持っているか？
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity;
};

// --- クエストの種類 ---
UENUM(BlueprintType)
enum class EQuestType : uint8
{
	Kill        UMETA(DisplayName = "討伐クエスト"),
	Gather      UMETA(DisplayName = "収集クエスト"),
	Delivery    UMETA(DisplayName = "お使い・会話クエスト")
};

// --- クエストの進行状態 ---
UENUM(BlueprintType)
enum class EQuestStatus : uint8
{
	NotStarted          UMETA(DisplayName = "未受注"),
	InProgress          UMETA(DisplayName = "進行中"),
	ObjectiveCleared    UMETA(DisplayName = "条件達成（報告待ち）"),
	Completed           UMETA(DisplayName = "完了（報酬受取済）")
};

// --- クエストの基本データ（データテーブル用） ---
USTRUCT(BlueprintType)
struct FQuestData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Info")
	FText QuestName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Info")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
	EQuestType QuestType = EQuestType::Kill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
	FName TargetID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Objective")
	int32 RequiredAmount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Reward")
	int32 RewardExperience = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Reward")
	int32 RewardGil = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Reward")
	FName RewardItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Reward")
	int32 RewardItemAmount = 0;
};

// --- プレイヤーが保持するクエスト進行データ ---
USTRUCT(BlueprintType)
struct FQuestProgress
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest|Progress")
	FName QuestID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest|Progress")
	EQuestStatus Status = EQuestStatus::NotStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest|Progress")
	int32 CurrentAmount = 0;

	FQuestProgress() {}

	FQuestProgress(FName InQuestID)
	{
		QuestID = InQuestID;
		Status = EQuestStatus::InProgress;
		CurrentAmount = 0;
	}
};