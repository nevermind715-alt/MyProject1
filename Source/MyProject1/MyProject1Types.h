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

// --- 追加：選択肢の文字色 ---
UENUM(BlueprintType)
enum class EDialogChoiceColor : uint8
{
	White   UMETA(DisplayName = "白（通常）"),
	Red     UMETA(DisplayName = "赤（危険・警告）"),
	Blue    UMETA(DisplayName = "青（冷静・知性）"),
	Purple  UMETA(DisplayName = "紫（特殊・狂気）")
};

// --- 特殊技の発動条件 ---
UENUM(BlueprintType)
enum class ESpecialCondition : uint8
{
	None            UMETA(DisplayName = "条件なし（手動用）"),
	HPBelowPercent  UMETA(DisplayName = "HPが指定%以下のとき"),
	AttackCount     UMETA(DisplayName = "オートアタック指定回数ごと")
};

// --- 特殊技の追加効果の種類（将来用） ---
UENUM(BlueprintType)
enum class ESpecialEffectType : uint8
{
	None        UMETA(DisplayName = "なし"),
	Stun        UMETA(DisplayName = "スタン（行動停止）"),
	Knockback   UMETA(DisplayName = "ノックバック"),
	HealHP      UMETA(DisplayName = "HP吸収"),
	LowerDefense UMETA(DisplayName = "防御ダウン")
};

// --- 30日サイクルの状態 ---
UENUM(BlueprintType)
enum class ECycleState : uint8
{
	StateA      UMETA(DisplayName = "状態A"),
	StateB      UMETA(DisplayName = "状態B"),
	StateC      UMETA(DisplayName = "状態C"),
	StateD      UMETA(DisplayName = "状態D")
};

USTRUCT(BlueprintType)
struct FCyclePhaseSettings
{
	GENERATED_BODY()

	/** どの状態にするか（状態A、状態B、状態C） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle")
	ECycleState TargetState = ECycleState::StateA;

	/** 開始日（例：1） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "1"))
	int32 MinDay = 1;

	/** 終了日（例：10） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cycle", meta = (ClampMin = "1"))
	int32 MaxDay = 10;

	// 💡将来の拡張：ここに「この状態の時の攻撃力補正倍率」などを足すことも可能です！
};

// --- 特殊効果の設定構造体 ---
USTRUCT(BlueprintType)
struct FSpecialEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESpecialEffectType EffectType = ESpecialEffectType::None;

	/** 効果の強さや確率、持続時間など */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.0f;
};

// --- 特殊技1つ分の設定データ ---
USTRUCT(BlueprintType)
struct FSpecialAttackData
{
	GENERATED_BODY()

	/** 技の名前（ログ表示用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SkillName = TEXT("特殊技");

	/** 発動するモンタージュ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* Montage = nullptr;

	/** 発動する条件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ESpecialCondition Condition = ESpecialCondition::None;

	/** 条件の数値（HPなら50.0で50%以下、AttackCountなら3.0で3回ごと） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ConditionValue = 0.0f;

	/** 連続発動を防ぐためのクールダウン（秒）。HP条件の場合に毎フレーム発動するのを防ぎます */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Cooldown = 10.0f;

	/** ダメージ倍率（1.5なら通常の1.5倍の威力） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float DamageMultiplier = 1.0f;

	/** クリティカル率のボーナス（20.0なら+20%） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float CriticalRateBonus = 0.0f;

	/** 特殊効果のリスト（＋ボタンで複数追加可能） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TArray<FSpecialEffectInfo> SpecialEffects;
};

USTRUCT(BlueprintType)
struct FLootItem
{
	GENERATED_BODY()

	// 落とすアイテムのID（FItemDataの行名）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	// ドロップする確率（%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DropRate = 5.0f;

	// 落とす個数
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;
};

// --- 追加：ジョブの基本データ（データテーブル用） ---
USTRUCT(BlueprintType)
struct FJobAttributes : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData|Base Stats")
	float BaseHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData|Base Stats")
	float BaseSTR = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData|Base Stats")
	float BaseVIT = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData|Base Stats")
	float BaseDEX = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData|Base Stats")
	float BaseAGI = 10.f;

	// そのジョブ（敵）が共通で落とすアイテムリスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData|Loot")
	TArray<FLootItem> BaseLootTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	EWeaponType DefaultWeapon = EWeaponType::HandToHand;

	// このジョブ（武器）の基本アニメーション（AnimBP）を指定する枠
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSubclassOf<class UAnimInstance> AnimBlueprintClass;

	// そのジョブ（敵）の本体メッシュ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<USkeletalMesh> CharacterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<USkeletalMesh> HairMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<USoundBase> AttackSound;

	// そのジョブが装備する武器のメッシュ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;

	// 武器の大きさ（デフォルトは 1.0, 1.0, 1.0）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	FVector WeaponScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	FName WeaponSocketName = FName("hand_r_socket");

	// そのジョブが装備する武器のメッシュ (Static用：バットや剣などはこちらを使うことが多いです)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TSoftObjectPtr<class UStaticMesh> StaticWeaponMesh;

	// ジョブごとの攻撃モーションセット（必要に応じて）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TArray<UAnimMontage*> AttackMontages;

	// ジョブが持つ特殊技（ウェポンスキル）のリスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JobData")
	TArray<FSpecialAttackData> SpecialAttacks;

	/** クリティカルヒット時の音 */
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
	float MaxStamina = 100.0f;

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

	// --- 獲得済みのストーリーフラグや条件のリスト ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Flags")
	TArray<FName> UnlockedFlags;
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

//どのステータスを変化させるかの定義
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
	AttackPower UMETA(DisplayName = "攻撃力"),
	Favor       UMETA(DisplayName = "好感度 (Favor)"),
	Fame        UMETA(DisplayName = "名声 (Fame)"),
	Charm       UMETA(DisplayName = "魅力 (Charm)"),
	CustomExtraStat UMETA(DisplayName = "カスタムステータス (ExtraStats)")
};

// --- バフ・デバフ専用のデータ（データテーブル用） ---
USTRUCT(BlueprintType)
struct FBuffData : public FTableRowBase
{
	GENERATED_BODY()

	// 画面に表示するバフの名前（「攻撃力アップ」「毒」など）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
	FString BuffName;

	// 画面上部に並べる専用アイコン画像
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
	UTexture2D* BuffIcon = nullptr;

	// ※将来的に「毒のダメージ量」や「キラキラ光るエフェクトの色」などもここに足せます
};

USTRUCT(BlueprintType)
struct FItemEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETargetStat TargetStat = ETargetStat::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "TargetStat == ETargetStat::CustomExtraStat"))
	FName ExtraStatName;

	/** 効果量（回復なら正、ダメージなら負） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectAmount = 0.0f;

	/** 効果時間（0なら即時、0より大きければリジェネやバフとして扱う想定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff")
	FName BuffID;
};

// ==========================================
// ▼ アビリティ（魔法・スキル）システム用の定義 ▼
// ==========================================

// --- アビリティの使用可能なタイミング ---
UENUM(BlueprintType)
enum class EAbilityUsageContext : uint8
{
	Anywhere      UMETA(DisplayName = "いつでも（ケアルなど）"),
	CombatOnly    UMETA(DisplayName = "戦闘中のみ（ウェポンスキルなど）"),
	FieldOnly     UMETA(DisplayName = "非戦闘時のみ（テレポなど）")
};

// --- アビリティの対象 ---
UENUM(BlueprintType)
enum class EAbilityTargetType : uint8
{
	Self          UMETA(DisplayName = "自分自身"),
	AllySingle    UMETA(DisplayName = "味方単体"),
	EnemySingle   UMETA(DisplayName = "敵単体"),
	AreaOfEffect  UMETA(DisplayName = "範囲（AoE）")
};

// --- アビリティの設計図（データテーブルの1行分） ---
USTRUCT(BlueprintType)
struct FAbilityData : public FTableRowBase
{
	GENERATED_BODY()

	// アビリティの名前
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AbilityName;

	// 説明文
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText Description;

	// アイコン画像
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTexture2D* Icon = nullptr;

	// --- 発動条件 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	EAbilityUsageContext UsageContext = EAbilityUsageContext::Anywhere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	EAbilityTargetType TargetType = EAbilityTargetType::EnemySingle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	float AbilityRange = 150.0f;

	// =========================================================
	// 攻撃アビリティ専用のパラメータ
	// =========================================================
	/** このアビリティが直接ダメージを与える「攻撃技」かどうか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bIsAttackAbility = false;

	/** 基本ダメージ（D値） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (EditCondition = "bIsAttackAbility"))
	float BaseDamage = 10.0f;

	/** 攻撃回数（ヒット数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (EditCondition = "bIsAttackAbility", ClampMin = "1"))
	int32 HitCount = 1;

	/** クリティカル発生率（％） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage", meta = (EditCondition = "bIsAttackAbility"))
	float CriticalRate = 5.0f;
	// =========================================================

	// --- 消費コスト ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	float CostStamina = 0.0f;

	// ウェポンスキルなどに使用するTP（タクティカルポイント）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	int32 CostTP = 0;

	// --- 時間管理 ---
	// 詠唱にかかる時間（秒）。0なら即時発動（ウェポンスキルなど）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float CastTime = 2.0f;

	// 次に使えるようになるまでのクールタイム（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float RecastTime = 10.0f;

	// --- 演出と効果 ---
	// 詠唱中にループ再生するモーション（魔法陣を出すなど）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	class UAnimMontage* CastMontage = nullptr;

	// 発動した瞬間に再生するモーション（剣を振り下ろす、杖を掲げるなど）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	class UAnimMontage* ExecuteMontage = nullptr;

	// 詠唱中に発生するエフェクト（足元の魔法陣やオーラなど。今回は枠だけ用意）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Effect")
	TSoftObjectPtr<class UNiagaraSystem> CastEffect;

	// 発動時（ダメージ発生時）に対象に発生するエフェクト（爆発や斬撃など）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual|Effect")
	TSoftObjectPtr<class UNiagaraSystem> ExecuteEffect;

	// 既存のアイテム効果（FItemEffect）の仕組みを完全に使い回して、何が起きるかを定義します
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TArray<FItemEffect> Effects;
};

// --- アクティブなバフ・デバフを管理する構造体 ---
USTRUCT(BlueprintType)
struct FActiveBuff
{
	GENERATED_BODY()

	// バフの名前（"プロテス" など。解除時の検索用にも使います）
	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	FString BuffName;

	// 画面に表示するアイコン画像
	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	UTexture2D* BuffIcon = nullptr;

	// ゲーム内時間（GetTimeSeconds）を基準にした、このバフが切れる時刻
	UPROPERTY(BlueprintReadOnly, Category = "Buff")
	float ExpirationTime = 0.0f;
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

	// 落とすアイテムのID（FItemDataの行名）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	// ドロップする確率（%）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DropRate = 5.0f;

	// 落とす個数
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity = 1;

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

// --- 会話アクションの種類 ---
UENUM(BlueprintType)
enum class EDialogActionType : uint8
{
	None            UMETA(DisplayName = "何もしない（次の会話へ）"),
	AcceptQuest     UMETA(DisplayName = "クエストを受注する"),
	ReportQuest     UMETA(DisplayName = "クエストを報告する"),
	ChangeStat      UMETA(DisplayName = "ステータスを変化させる"),
	GiveItem        UMETA(DisplayName = "アイテムを渡す/奪う"),
	AddFlag         UMETA(DisplayName = "フラグを獲得する"),
	RemoveFlag      UMETA(DisplayName = "フラグを消去する"),
	Warp            UMETA(DisplayName = "ワープを実行する"),
	Close           UMETA(DisplayName = "会話を終了する")
};

// --- 選択肢1つ分のデータ ---
USTRUCT(BlueprintType)
struct FDialogChoice
{
	GENERATED_BODY()

	// 選択肢のテキスト（「わかった！」「断る」など）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FString ChoiceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Condition")
	EDialogChoiceColor ChoiceColor = EDialogChoiceColor::White;

	// 要求される精神値（Mentalがこの数値以上ないと選べない）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Condition")
	float RequiredMental = 0.0f;

	// 必須フラグ（空欄ならフラグ不要。文字が入っていれば、そのフラグを持っている時だけ選べる）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Condition")
	FName RequiredFlag;

	// 選んだ時に何をするか
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	EDialogActionType ActionType = EDialogActionType::None;

	// アクションの引数（QuestIDの文字列や、好感度の増減数値など）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FString ActionPayload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	ETargetStat StatToChange = ETargetStat::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog", meta = (EditCondition = "StatToChange == ETargetStat::CustomExtraStat"))
	FName ExtraStatName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	float StatChangeAmount = 0.0f;

	// 次に飛ぶ会話のID（会話を続ける場合）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FName NextDialogID;
};

// --- 会話全体のデータ（データテーブル用） ---
USTRUCT(BlueprintType)
struct FDialogData : public FTableRowBase
{
	GENERATED_BODY()

	// 誰が喋っているか（空欄ならNPCの名前を使う想定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FString SpeakerName;

	// セリフ本文
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog", meta = (MultiLine = true))
	FText DialogText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Audio")
	class USoundBase* DialogSE = nullptr;

	// 再生するボイス音声
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Audio")
	class USoundBase* DialogVoice = nullptr;

	// 再生するモーション（頷く、怒るなど）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog|Animation")
	class UAnimMontage* DialogEmote = nullptr;

	// このセリフを読み終わった時に会話を明示的に終了するフラグ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	bool bEndDialog = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	FName NextDialogID;

	// 選択肢のリスト（配列）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
	TArray<FDialogChoice> Choices;
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
// --- クエストの基本データ（データテーブル用） ---
USTRUCT(BlueprintType)
struct FQuestData : public FTableRowBase
{
	GENERATED_BODY()

	// --- クエストの受注条件 ---
	// 空欄なら誰でも受注可能。文字が入っていれば、そのフラグ（称号）が必要
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Condition")
	FName RequiredFlag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Info")
	FText QuestName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Info", meta = (MultiLine = true))
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

	// --- クエストのクリア報酬（称号・フラグ） ---
	// 空欄なら何もしない。文字が入っていればクリア時にフラグを付与
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Reward")
	FName RewardFlag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Audio")
	class USoundBase* AcceptSound = nullptr; // 受注時の音

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Audio")
	class USoundBase* CompletionSound = nullptr; // 完了時の音

	// --- 掲載場所の設定 ---
	// Trueならクエストボードに表示、FalseならNPCから直接受注する専用クエスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Display")
	bool bIsBoardQuest = true;

	// どのボードに掲載するか（例："TownA", "Guild" など。ボード側の設定と合わせる）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Display", meta = (EditCondition = "bIsBoardQuest"))
	FName BoardGroupID;

	// --- 繰り返し（デイリー等）設定 ---
	// 再受注可能かどうか
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repeat")
	bool bIsRepeatable = false;

	// クリアしてから何日経過で再受注できるか（1なら翌日。0なら即時）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repeat", meta = (EditCondition = "bIsRepeatable"))
	int32 CooldownDays = 1;
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

// --- クリアしたクエストの履歴用構造体 ---
USTRUCT(BlueprintType)
struct FCompletedQuestInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest|Progress")
	FName QuestID;

	// クリアした時の「累計日数」を記録しておく（クールタイム判定用）
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest|Progress")
	int32 CompletedTotalDays = 0;
};


// --- ワープポータルの種類（今後のマップ配置用） ---
UENUM(BlueprintType)
enum class EWarpPortalType : uint8
{
	TouchToWarp     UMETA(DisplayName = "触れると即ワープ"),
	InteractToWarp  UMETA(DisplayName = "調べてワープ（確認なし）"),
	ConfirmToWarp   UMETA(DisplayName = "調べてワープ（「移動しますか？」の確認あり）")
};

// --- ワープ先の基本データ（データテーブル用） ---
USTRUCT(BlueprintType)
struct FWarpDestination : public FTableRowBase
{
	GENERATED_BODY()

	// 飛ぶ先のマップ名（現在のマップと同じなら空欄でもOK、という仕様にします）
	// ※「Map_Town」のように、レベルファイルの名前と一致させる必要があります
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	FName TargetLevelName;

	// ワープ後の座標と向き（X, Y, Z と 回転）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	FTransform DestinationTransform;

	// UIに表示する名前（「王都ルシス・城門前」など）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	FText DisplayName;

	// 飛ぶために必要なフラグ（空欄なら無条件。フラグ名があれば、それを持っていないと飛べない）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	FName RequiredFlag;
};

// --- 装備スロットの定義 ---
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	Hair          UMETA(DisplayName = "髪型"),
	Head          UMETA(DisplayName = "頭防具"),
	Torso         UMETA(DisplayName = "胴防具"),
	InnerUpper    UMETA(DisplayName = "上半身インナー"),
	InnerLower    UMETA(DisplayName = "下半身インナー"),
	Arms          UMETA(DisplayName = "腕防具"),
	Hands         UMETA(DisplayName = "手防具（手袋・小手）"),
	Legs          UMETA(DisplayName = "脚防具"),
	Feet          UMETA(DisplayName = "足防具"),
	Neck          UMETA(DisplayName = "首装備（Static）"),
	Wrist         UMETA(DisplayName = "腕輪（Static）"),
	Ankle         UMETA(DisplayName = "足輪（Static）"),
	Weapon        UMETA(DisplayName = "メイン武器"),
	Extra1        UMETA(DisplayName = "特殊枠1（非表示）"),
	Extra2        UMETA(DisplayName = "特殊枠2（非表示）"),
	Extra3        UMETA(DisplayName = "特殊枠3（非表示）"),
	Extra4        UMETA(DisplayName = "特殊枠4（非表示）"),
	Extra5        UMETA(DisplayName = "特殊枠5（非表示）"),
	Max           UMETA(Hidden)
};

// --- 装備品データの構造体 ---
USTRUCT(BlueprintType)
struct FEquipmentData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (DisplayName = "開発用メモ（アイテム名）"))
	FString DevMemo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EEquipmentSlot TargetSlot = EEquipmentSlot::Torso;

	// 柔らかい服や変形する装備用（Tシャツなど）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TSoftObjectPtr<USkeletalMesh> EquipSkeletalMesh;

	// 固いアクセサリー用（ネックレス腕輪・足輪など）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	TSoftObjectPtr<UStaticMesh> EquipStaticMesh;

	// ステータス補正値
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
	int32 AddSTR = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
	int32 AddDEF = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
	int32 AddMND = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offset")
	float HeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offset")
	FRotator AnkleRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Offset")
	FRotator ToeRotationOffset = FRotator::ZeroRotator;
};

// --- ExtraStatsとモーフターゲットを連動させるための設定 ---
USTRUCT(BlueprintType)
struct FExtraStatMorphConfig
{
	GENERATED_BODY()

	// 連動させたいメッシュ側のモーフターゲット名
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morph")
	FName MorphTargetName;

	// ステータスの数値をモーフ値(0.0〜1.0)に変換するための基準値（最大値）
	// 例：100.0 に設定すると、ステータスが 50 の時にモーフは 0.5 になります。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morph")
	float MaxStatValue = 100.0f;
};