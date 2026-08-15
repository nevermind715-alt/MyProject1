// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Kismet/KismetMathLibrary.h" // これを追加
#include "MyProject1Types.h" // これを追加！
#include "InventoryComponent.h"
#include "InputActionValue.h"
#include "RpgCharacterInterface.h"
#include "NPCAIInterface.h"
#include "MyProject1Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UDialogComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UCableComponent;
class AShopNPCBase;
class UAnimInstance;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChangedSignature, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChangedDelegate, float, CurrentStamina, float, MaxStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLogScrollToBottomSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeathSignature, AActor*, DeadActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsUpdatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBuffListChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkinOverlayUIChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAdventurerRankChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFlagAddedSignature, FName, FlagName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFlagRemovedSignature, FName, FlagName);


// クリティカルダメージを識別するための専用クラス
UCLASS()
class UCriticalDamageType : public UDamageType
{
	GENERATED_BODY()
};

UCLASS(abstract)
class AMyProject1Character : public ACharacter, public IRpgCharacterInterface, public INPCAIInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	// --- 水に入った時の処理 ---
		// 水中での歩行スピード
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WaterWalkSpeed = 200.0f;

	// 陸上での基本歩行スピード（パトロール速度とは別の、プレイヤーの基本速度）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LandWalkSpeed = 500.0f;

	

	/** ズーム操作用の入力アクション（Enhanced Input用） */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ZoomAction;

	/** 非戦闘時のHP自動回復の間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Regeneration")
	float AutoRecoveryInterval = 5.0f;

	/** 1回あたりの回復量（最大HPに対する割合 0.05 = 5%） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Regeneration")
	float AutoRecoveryRate = 0.05f;

	/** 戦闘時の1秒あたりのスタミナ回復量（毎秒どれくらい回復するか） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Regeneration")
	float StaminaRecoveryCombat = 10.0f;

	/** 非戦闘時の1秒あたりのスタミナ回復量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Regeneration")
	float StaminaRecoveryField = 25.0f;

	/** 自動回復を管理するタイマーハンドル */
	FTimerHandle TimerHandle_AutoRecovery;

	/** 定期的に呼ばれる回復関数 */
	void HandleAutoRecovery();

	/** 疲労度を1秒ごとに計算するためのタイマー */
	FTimerHandle TimerHandle_FatigueUpdate;

	/** 1秒ごとに呼ばれる疲労度計算関数 */
	void HandleFatigueTick();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Regeneration")
	float AutoRecoveryStartDelay = 10.0f;

	/** 最後に戦闘行動（ターゲット、攻撃、被弾）があった時刻 */
	double LastCombatTime = 0.0;

	/** ログをスクロールさせるためのイベント（BPで実装） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|UI")
	void OnScrollLog(float ScrollDelta);

	FTimerHandle LogScrollTimerHandle;

	void HandleLogAutoScroll();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|UI")
	void OnRequestLogScrollToBottom();

	/** スペースキーが離された時の通知用（BPで実装） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|UI")
	void OnSpaceActionReleased();

	/** WASD移動またはスペースキーの入力があった時、ログウィンドウを再表示させるための通知（BPで実装） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|UI")
	void OnRequestShowLogWindow();

	/** 入力をフックするための関数 */
	void HandleJumpCompleted();

	/** カメラの最短距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MinTargetArmLength = 250.0f;

	/** カメラの最長距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MaxTargetArmLength = 1000.0f;

	/** 1回のホイール操作で動く距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ZoomStep = 25.0f;

	/** ズーム入力を処理する関数 */
	void ZoomCamera(const struct FInputActionValue& Value);

	void ExpireItemBuff(FString ItemName, TArray<FItemEffect> Effects, int32 StackID);

	// --- まばたき・目閉じ（モーフターゲット）制御 ---

	/** 目を閉じるモーフターゲットの名前（VRMの場合は "Blink" や "Fcl_EYE_Close" など） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	FName BlinkMorphName = TEXT("Blink");

	/** まばたきする間隔の最小値（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float BlinkIntervalMin = 2.0f;

	/** まばたきする間隔の最大値（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float BlinkIntervalMax = 6.0f;

	/** まばたきの開閉スピード */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Animation")
	float BlinkSpeed = 15.0f;

	// 内部処理用の変数
	float CurrentBlinkValue = 0.0f;
	float TimeUntilNextBlink = 0.0f;
	bool bIsBlinking = false;
	bool bIsClosingEyes = false;

	/** 毎フレーム呼び出されてまばたきを計算する関数 */
	void UpdateBlink(float DeltaTime);

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicBodyMaterialInstance;

	
	

public:
	/** Constructor */
	AMyProject1Character();

	// キャラクターの名前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FString CharacterName = "Senshi";

	// UIに名前を通知するイベント
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateHealthWidgetName(const FString& NewName);

	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnStatsUpdatedSignature OnStatsUpdatedDelegate;

	// 現在かかっているバフのリスト
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Buff")
	TArray<FActiveBuff> ActiveBuffs;

	// 重複可のバフを連続使用した際に、使用ごとへ振る通し番号（次に使う値）
	int32 NextBuffStackID = 1;

	// バフが増減した時にUIへ知らせる合図
	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnBuffListChangedSignature OnBuffListChangedDelegate;

	
	UFUNCTION(BlueprintCallable, Category = "Combat|Stats")
	void NotifyStatsChanged() override;

	// --- アビリティコンポーネント ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UAbilityComponent* AbilityComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Loot")
	TArray<FLootItem> PersonalLootTable;


	// --- フラグ管理 ---
	/** フラグが追加された瞬間に発火する。会話フラグをトリガーに、同じレベル内でワープや
	    クエストアイテムポイントをレベル再読み込みなしにその場で出現させたい時に購読する */
	UPROPERTY(BlueprintAssignable, Category = "RPG Combat|Flags")
	FOnFlagAddedSignature OnFlagAdded;

	/** フラグが削除された瞬間に発火する。表示切替（VisibilityFlag）などフラグ追加時と対になる
	    後片付け・リセット処理を、レベル再読込なしにその場で反映させたい時に購読する */
	UPROPERTY(BlueprintAssignable, Category = "RPG Combat|Flags")
	FOnFlagRemovedSignature OnFlagRemoved;

	/** フラグ（条件）を獲得する */
	UFUNCTION(BlueprintCallable, Category = "RPG Combat|Flags")
	void AddFlag(FName FlagName) override;

	// フラグを消去する
	UFUNCTION(BlueprintCallable, Category = "RPG Combat|Flags")
	void RemoveFlag(FName FlagName) override;

	/** 指定したフラグ（条件）を持っているか確認する */
	UFUNCTION(BlueprintPure, Category = "RPG Combat|Flags")
	bool HasFlag(FName FlagName) const override;

	virtual FCharacterStats& GetCharacterStats() override { return MyStats; }
	virtual UQuestComponent* GetQuestComponent() const override { return QuestComp; }

	// ギルドNPCへの申請により、冒険者等級の昇格を試みる（IRpgCharacterInterface実装）
	virtual bool TryRankUp() override;

	// --- 疲労度（Energy）設定 ---

	/** ゲーム内の1日あたりに溜まる蓄積疲労度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue")
	float FatigueIncreasePerInGameDay = 20.0f;

	/** オートアタック1回ごとに増える疲労度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue")
	float FatigueIncreasePerAttack = 1.0f;

	/** 戦闘中、1秒間にジワジワ増える疲労度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue")
	float FatigueIncreasePerSec = 0.5f;

	/** 非戦闘中、1秒間に回復（減少）する疲労度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue")
	float FatigueDecreasePerSec = 2.0f;
	
	/** 疲労度を安全に増減させ、UIを更新する関数 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Fatigue")
	void UpdateEnergy(float Amount);

	// --- 疲労度によるペナルティ設定 ---

	/** 軽度の疲労（デバフ①）が始まる数値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue Penalty")
	float FatigueThreshold1 = 50.0f;

	/** 重度の疲労（デバフ②）が始まる数値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue Penalty")
	float FatigueThreshold2 = 90.0f;

	/** 軽度疲労時の攻撃力ダウン率（0.05 = 5%ダウン） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue Penalty")
	float FatigueAttackPenalty1 = 0.05f;

	/** 重度疲労時の攻撃力ダウン率（0.10 = 10%ダウン） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue Penalty")
	float FatigueAttackPenalty2 = 0.10f;

	/** 重度疲労時の攻撃速度ダウン率（0.05 = 間隔が5%延長されて遅くなる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Fatigue Penalty")
	float FatigueSpeedPenalty2 = 0.05f;

	/** 現在の疲労度を加味した最終的な【攻撃力】を返す関数 */
	UFUNCTION(BlueprintPure, Category = "Combat|Fatigue Penalty")
	float GetModifiedAttackPower() const;

	/** 現在の疲労度を加味した最終的な【攻撃間隔】を返す関数 */
	UFUNCTION(BlueprintPure, Category = "Combat|Fatigue Penalty")
	float GetModifiedAttackSpeed() const;

	/** 特殊技（ウェポンスキル等）を実行する関数 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Special")
	void PerformSpecialAttack(UAnimMontage* SpecialMontage);

	/** 武器を表示するためのコンポーネント */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMeshComp;

	/** 武器を表示するためのコンポーネント（Static用） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* StaticWeaponMeshComp;

	/** 操作をロックするかどうかのフラグ */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
	bool bIsInputLocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Cinematic")
	bool bIsInCutscene = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleMenuAction;

	/** 操作ロックを切り替える関数（BPから呼び出し可能） */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputLocked(bool bLocked) override;

	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	bool IsReadingOldLogs() const;

	/** NPCとしてログに台詞を表示する */
	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	void TalkToLog(const FString& Message);

	// 2. WidgetからBindできるように UPROPERTY(BlueprintAssignable) をつける
	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnHPChangedSignature OnHPChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnStaminaChangedDelegate OnStaminaChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnLogScrollToBottomSignature OnLogScrollToBottomDelegate;

	/** HPを増減させる汎用関数（正の数で回復、負の数でダメージ） */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float UpdateHealth(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ToggleCombatMode();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Buff")
	void ApplyItemBuff(FString ItemName, UTexture2D* Icon, const TArray<FItemEffect>& Effects, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Character|Audio")
	void PlayFootstepSound();

	/** 材質ごとの音を登録するマップ（データテーブルでも可） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Audio")
	TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> FootstepSounds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Audio")
	class USoundAttenuation* FootstepAttenuation;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCharacterDeathSignature OnDeathDelegate;

	// ==========================================
	// 装備システム（見た目用コンポーネント）
	// ==========================================

	// --- 柔らかい装備（SkeletalMesh） ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* HairMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* FaceMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* HeadMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* TorsoMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* InnerUpperMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* InnerLowerMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* WaistMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* HandsMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* LegsMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* FeetMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* WristSkeletalMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* NeckSkeletalMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* AnkleSkeletalMeshComp;

	/** 未設定（None）ならAnkleSkeletalMeshCompは今まで通りLeader Pose Componentで本体に追従する（＝現状維持、何も壊れない）。
	 *  設定すると、AnkleSkeletalMeshCompはLeader Pose ComponentをやめてこのAnimBPで動くようになる。
	 *  このABP側で「Copy Pose from Mesh（本体Meshから）」した後、J_Bip_L/R_Footに対して
	 *  CurrentAnkleRotationOffset * -1 を追加のTransform(Modify)Boneで加算すれば、
	 *  本体ABP側のヒール補正で足首装備だけ一緒に傾いてしまう問題を、同じボーン空間内で正しく打ち消せる
	 *  （Leader Pose Component中はFollower自身のAnimGraphが評価されないため、Leader Poseを外す必要がある）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Offset")
	TSubclassOf<UAnimInstance> AnkleAccessoryAnimClass;

	/** ACharacter::GetMesh()はBlueprintに公開されていない（BlueprintCallableが付いていない）ため、
	 *  足首装備用ABPの Copy Pose from Mesh に本体メッシュを渡すための薄いラッパー。 */
	UFUNCTION(BlueprintPure, Category = "Character")
	USkeletalMeshComponent* GetCharacterMesh() const { return GetMesh(); }

	// --- 特殊枠（SkeletalMesh・ピアス等） ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* Extra1MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* Extra2MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* Extra3MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* Extra4MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	USkeletalMeshComponent* Extra5MeshComp;

	/** 現在適用されている靴の高さ補正値（脱ぐ時に元に戻すため） */
	float CurrentShoesOffset = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Offset")
	FRotator CurrentAnkleRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Offset")
	FRotator CurrentToeRotationOffset = FRotator::ZeroRotator;


	// --- FRotator ToeRotation を引数に追加 ---
	void ApplyShoesOffset(bool bEquip, float Offset = 0.0f, FRotator RotationOffset = FRotator::ZeroRotator, FRotator ToeRotation = FRotator::ZeroRotator);

	// 現在装備中の胴・腰装備のbHideInnerUpper/bHideInnerLower設定を見て、
	// インナー(InnerUpperMeshComp/InnerLowerMeshComp)の表示・非表示を更新する
	void RefreshInnerVisibility();


	// --- 固いアクセサリー（StaticMesh） ---
	// ※ソケットに直接アタッチします

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	UStaticMeshComponent* NeckMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	UStaticMeshComponent* WristMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Components")
	UStaticMeshComponent* AnkleMeshComp;

	// ==========================================
	// 装備システムのデータと関数
	// ==========================================

	// 現在装備しているアイテムのデータテーブルRowNameを保持
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TMap<EEquipmentSlot, FName> CurrentEquippedItems;

	// 直近でRefreshEquipmentStatsが装備から加算したExtraStats量（再計算時に差分だけ戻すための記録。ExtraStatsはアイテム消費等でも永続的に増減する値なので、単純な再計算だと二重加算になってしまうため）
	UPROPERTY()
	TMap<FName, float> EquipmentExtraStatBonuses;

	// 直近でRefreshEquipmentStatsが病気・ケガ／タトゥー／ピアスから加算したExtraStats量（装備分とは別の記録が必要。同じ理由で二重加算防止のため）
	UPROPERTY()
	TMap<FName, float> SkinOverlayExtraStatBonuses;

	// アイテムIDと装備データの両方を受け取るようにします
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void EquipItem(FName ItemID, FEquipmentData EquipData);

	// 外す部位を指定して受け取ります
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipItem(EEquipmentSlot TargetSlot);

	
	// 全装備のステータス補正を再計算して適用する
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshEquipmentStats();

	// 装備（DT_EquipmentsのStatModifiers）によるステータス補正を反映するかどうか。
	// Player本体ではtrueのまま、AQuestNPCBaseがfalseに上書きすることで、
	// NPCはEquipItem()で見た目（メッシュ・オーバーレイ）だけ変わり、MyStatsは変化しなくなる
	virtual bool ShouldApplyEquipmentStatBonuses() const { return true; }

	// ==========================================
	// 拘束具（足枷等）による移動制限
	// ==========================================

	/** 現在、移動制限のかかる装備（足枷等）を身につけているか */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement|Restraint")
	bool bIsMovementRestricted = false;

	/** 移動制限中の速度上限（GameInstanceの設定値から解決される） */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement|Restraint")
	float RestrainedSpeedCap = 0.0f;

	/** 現在、拘束用ABPに差し替え中か */
	bool bUsesRestrainedAnimBlueprint = false;

	/** 拘束用ABPに差し替える前の、元々のAnimBlueprintクラス（解除時に復元するため） */
	UPROPERTY()
	TSubclassOf<UAnimInstance> DefaultAnimBlueprintClass;

	/** 現在装備中のアイテムをすべて確認し、移動制限とアニメーション差し替えの状態を更新する（EquipItem/UnequipItemから呼ばれる） */
	UFUNCTION(BlueprintCallable, Category = "Movement|Restraint")
	void RefreshMovementRestriction();

	// 装備データテーブルの参照
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	UDataTable* EquipmentDataTable;

	
	// バフ専用データテーブルの参照
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Buff")
	UDataTable* BuffDataTable;

	
	/**
	 * 指定した部位（スロット）に装備されているアイテムのIDを取得します。
	 * UIのボタンに「Tシャツ」などの名前を表示する際に使います。
	 * 何も装備していない場合は "None" が返ります。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	FName GetEquippedItemID(EEquipmentSlot Slot);

	/** 動的な鎖を表現するコンポーネント */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Cable")
	UCableComponent* EquipmentCableComp;
		
	/** 鎖の始点スケールバグを中継して消し去るための、スケール1の透明なダミーコンポーネント */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Cable")
	USceneComponent* CableDummyStart;

	/** 鎖の終点スケールバグを中継して消し去るための、スケール1の透明なダミーコンポーネント */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Cable")
	USceneComponent* CableDummyEnd;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Cable")
	UCableComponent* EquipmentCableComp_Hands;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Cable")
	USceneComponent* CableDummyStart_Hands;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Cable")
	USceneComponent* CableDummyEnd_Hands;

	// 手・腕用の鎖の追従状態を管理する変数群
	FName CurrentCableSourceSocket_Hands;
	UPROPERTY()
	USceneComponent* CurrentCableSourceComponent_Hands;

	FName CurrentCableTargetSocket_Hands;
	UPROPERTY()
	USceneComponent* CurrentCableTargetComponent_Hands;

	/** 武器の鉄球など、鎖の先端にぶら下がるサブメッシュ（スタティック） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponSubMeshComp;

	/** 現在の鎖の始点ソケット名（毎フレームの追従バグ回避用） */
	FName CurrentCableSourceSocket;

	/** 現在の鎖の始点コンポーネント */
	UPROPERTY()
	USceneComponent* CurrentCableSourceComponent;

	/** 現在の鎖の終点ソケット名（右足用） */
	FName CurrentCableTargetSocket;

	/** 現在の鎖の終点コンポーネント */
	UPROPERTY()
	USceneComponent* CurrentCableTargetComponent;

	/** ショップやNPCから呼ばれる、タトゥー/傷跡/治療/ピアスの購入・追加を試みる共通窓口 */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay|Shop")
	void TryAddSkinOverlay(FName RowName, bool bIsShopPurchase, int32 OverridePrice = 0, FString DisplayName = TEXT(""), EShopModeCategory ShopCategory = EShopModeCategory::Tattoo);

	/** ショップやNPCから呼ばれる、タトゥー/傷跡/治療/ピアスの消去を試みる共通窓口 */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay|Shop")
	void TryRemoveSkinOverlay(FName RowName, bool bIsShopPurchase, int32 OverridePrice = 0, FString DisplayName = TEXT(""), EShopModeCategory ShopCategory = EShopModeCategory::Tattoo);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Skin Overlay|Shop")
	class AShopNPCBase* ActiveShopNPC;

	UPROPERTY(BlueprintAssignable, Category = "Skin Overlay|UI")
	FOnSkinOverlayUIChangedSignature OnSkinOverlayUIChangedDelegate;

	/** ショップから通常のアイテム（消費アイテム・装備・素材など）を購入する共通窓口 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Shop")
	bool TryBuyItem(FName ItemID, int32 Amount = 1);

	/** ショップへ手持ちのアイテムを売却する共通窓口 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Shop")
	bool TrySellItem(FName ItemID, int32 Amount = 1);

protected:
	/** 鎖設定に基づいてコンポーネントの状態を再構築する内部関数 */
	void SetupCableSystem(const FCableAttachmentSettings& Settings, EEquipmentSlot Slot, USceneComponent* AssociatedComponent = nullptr);

	/** 現在有効な鎖を安全に消去する関数 */
	void ClearCableSystem(EEquipmentSlot Slot);

protected:

	void OnToggleMenuPressed();

	/** Space(JumpAction)：ターゲット中の対象に応じて会話開始/ショップ開始/戦闘開始を振り分ける */
	void OnActionKeyPressed();

public:
	/**
	 * OnActionKeyPressedで、CurrentTargetがQuestNPC/ShopNPC/Enemyのどれでもなかった場合に呼ばれる。
	 * BPI_InteractableはBlueprint専用インターフェースでC++から直接呼べないため、
	 * BP_Character側でEキーと同じ「Interact(Message)」ノードをTargetピンに繋いで実装する。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void BP_TryInteractWithTarget(AActor* Target);

protected:
	/** インプットのセットアップ */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** ゲーム開始時に呼ばれる */
	virtual void BeginPlay() override;

	/** 特殊技を発動中かどうかのフラグ */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	bool bIsUsingSpecialAttack = false;

	// 現在発動している特殊技モンタージュを記憶しておく変数
	UPROPERTY()
	UAnimMontage* ActiveSpecialMontage = nullptr;

	/** アニメーションモンタージュが終了した時に自動で呼ばれる関数 */
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	FSpecialAttackData CurrentExecutingSkillData;

	
public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	/** 攻撃開始ボタンを押してから実際にオートアタックが始まるまでの準備フラグ */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIsPreparingAttack = false;

	/** 準備（構え）時間の長さ（2.5秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackStartupDelay = 2.5f;

	/** 攻撃開始ボタンを押した時刻を記録 */
	double StartAttackTimestamp = 0.0;


public:

	/** ログメッセージをUIに送るためのイベント（BPで中身を書く） */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat|UI")
	void OnReceiveLogMessage(const FString& Message, ELogMessageType MessageType) override;

	/** 現在ターゲットしている敵（空っぽならターゲットなし） */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
	AActor* CurrentTarget;

	/** オートアタックを実行中かどうか（抜刀状態） */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIsAutoAttacking = false;

	/** オートアタックを開始する命令（メニューの攻撃ボタンから呼ぶ） */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void StartAutoAttack();

	// ★ UFUNCTIONを追加して、BPから呼べるようにする
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetCurrentTarget(AActor* NewTarget) override;

	// ターゲットを明示的に解除する関数
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CancelTarget() override;

	// BP側でマーカーの表示/非表示を制御するためのイベント
	// C++から呼び出して、BP側に処理を投げます
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnTargetUpdated(AActor* TargetActor, bool bIsTargeted);

	/** ターゲットできる最大距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float TargetingRange = 2000.0f;

	/** 会話やインタラクトが届く距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Interact")
	float InteractRange = 250.0f;

	/** HPバーを自動表示する範囲（cm）。1000〜1500くらいが目安 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|UI")
	float HPBarDisplayRange = 1000.0f;

	/** HPバーの表示/非表示を切り替えるためのイベント（BPで実装） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|UI")
	void OnHPBarVisibilityChanged(bool bVisible);

	/** 一番近い敵を探してターゲットする関数 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TargetNearestEnemy();

	/** ターゲットを順送り（サイクル）にする関数 */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CycleTarget();


	// ★追加：HPが変化した時に呼ばれるイベント（BPで実装する）
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnHPChanged(float CurrentHP, float MaxHP);

private:
		/** 前回の表示状態を覚えておく変数（毎フレームのUI更新を避けるため） */
		bool bLastHPBarVisibility = false;

protected:
	/** 攻撃を中断する関数（死亡時やターゲット喪失時に呼ぶ） */
	void StopAutoAttack(); // ← これが無いとエラーになるので追加！

public:
	// --- ダメージ処理関係 ---

	// ダメージを受けた時に呼ばれる関数（オーバーライド）
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 死亡しているか確認
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsDead() const override { return bIsDead; }

protected:
	// 死亡処理
	virtual void OnDeath();

	// 死亡フラグ（AnimBPから読めるように UPROPERTY を追加！）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsDead = false;

public:

	// --- 疑似飛行（ホバー）設定 ---
	/** 空中を飛ぶ敵などの場合、地上からどれくらいメッシュを浮かすか (0なら通常) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Hover")
	float HoverHeight = 0.0f;

	// --- パトロール設定 (Combatカテゴリ) ---

/** パトロールをするかどうか（Falseなら定点監視） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Patrol")
	bool bCanPatrol = true;

	/** パトロール範囲（半径） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Patrol")
	float PatrolRadius = 1000.0f;

	/** パトロール待機時間の最小値（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Patrol")
	float PatrolWaitMin = 2.0f;

	/** パトロール待機時間の最大値（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Patrol")
	float PatrolWaitMax = 5.0f;

	/**パトロール中の歩き速度(FF11風なら200~300くらい)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Patrol")
	float PatrolWalkSpeed = 250.0f;

	/** 発見時の走り速度 (通常は600くらい) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Patrol")
	float ChaseRunSpeed = 600.0f;



public:
	// --- AI 知覚設定 (Combat|Sensors) ---

/** 敵を見つける距離（cm）。これより遠くは気づかない */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sensors")
	float AISightRadius = 1500.0f;

	/** 敵を見失う距離（cm）。一度見つかったら、これ以上離れるまで追いかける（少し長めにするのがコツ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sensors")
	float AILoseSightRadius = 2000.0f;

	/** 視野角（度）。90度なら前だけ、180度なら真横まで、360度なら背中も見えます */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sensors")
	float AIVisionAngle = 90.0f;

	/** Trueなら、一度見つけたら距離や壁に関係なく絶対に見失わない（ボス用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sensors")
	bool bNeverLoseSight = false;

	/** 聴覚を有効にするかどうか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sensors")
	bool bAIEnableHearing = true;

	/** 聴覚の範囲。これを小さくすると足音などに気づかれにくくなります */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sensors")
	float AIHearingRange = 3000.0f;

public:
	// --- INPCAIInterface実装 ---
	// AMyAIController（汎用の徘徊・索敵AI）がこのクラスの型を知らなくても
	// 同じ設定値・関数を読めるようにするための橋渡し。中身は既存プロパティの単純な受け渡し
	UFUNCTION(BlueprintCallable, Category = "AI") virtual bool CanPatrol() const override { return bCanPatrol; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetPatrolWalkSpeed() const override { return PatrolWalkSpeed; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetChaseRunSpeed() const override { return ChaseRunSpeed; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetPatrolRadius() const override { return PatrolRadius; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetPatrolWaitMin() const override { return PatrolWaitMin; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetPatrolWaitMax() const override { return PatrolWaitMax; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetAISightRadius() const override { return AISightRadius; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetAILoseSightRadius() const override { return AILoseSightRadius; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetAIVisionAngle() const override { return AIVisionAngle; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual bool IsAIHearingEnabled() const override { return bAIEnableHearing; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual float GetAIHearingRange() const override { return AIHearingRange; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual bool IsActiveEnemy() const override { return bIsActiveEnemy; }
	UFUNCTION(BlueprintCallable, Category = "AI") virtual bool GetNeverLoseSight() const override { return bNeverLoseSight; }

protected:

	/** 死亡時に自動でActorを削除するかどうか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bDestroyOnDeath = true;

	/** 死亡してから削除されるまでの時間（秒）。FF11なら5秒〜10秒くらいが目安 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float DeathLifeSpan = 5.0f;

public:

	/** ロックオン時のカメラ回転速度（大きいほどキビキビ動く） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CameraLockSpeed = 5.0f;

	// Tick関数（毎フレーム呼ばれる関数）のオーバーライド
	// ※元々書いてある場合は追加しなくてOKです
	virtual void Tick(float DeltaTime) override;

public:
	// ここにステータスを追加
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Stats")
	FCharacterStats MyStats;

	// エディタ（BP）上で「ExtraStatsのキー名」と「モーフ設定」を紐付けるためのリスト
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Stats|Extra")
	TMap<FName, FExtraStatMorphConfig> ExtraStatMorphLinks;

	// ExtraStatの値を取得する
	UFUNCTION(BlueprintPure, Category = "RPG Stats|Extra")
	float GetExtraStat(FName StatName) const;

	// ExtraStatの値を直接上書き（Set）する
	UFUNCTION(BlueprintCallable, Category = "RPG Stats|Extra")
	void SetExtraStat(FName StatName, float Value);

	// ExtraStatの値を増減（Add/Sub）する（ダメージや回復のように使う）
	UFUNCTION(BlueprintCallable, Category = "RPG Stats|Extra")
	void AddExtraStat(FName StatName, float Amount);

	/** 現在のサイクルの状態（状態A, B, C） */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Time Cycle")
	ECycleState CurrentCycleState;

	/** 現在のサイクルにおける日数（1〜30） */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Time Cycle")
	int32 CurrentCycleDay;

	/** サイクルを計算して状態を更新する関数 */
	UFUNCTION(BlueprintCallable, Category = "Time Cycle")
	void UpdateCycleState();

protected:
		// モーフターゲットを更新する内部処理
		void UpdateMorphTargetFromStat(FName StatName, float Value);

public:
	// --- RPGシステム用の追加関数 ---

	/** 経験値を加算する関数。Blueprintからも呼び出せるようにします */
	UFUNCTION(BlueprintCallable, Category = "RPG Combat")
	void AddExperience(int32 Amount);

protected:
	/** レベルアップ時の計算を行う内部関数 */
	void LevelUp();

public:

	/** 攻撃が届く距離（ターゲット距離2000より短く、例えば150くらい） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackRange = 150.0f;

	/** 攻撃ミス時の共通SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sounds")
	USoundBase* GlobalMissSound;

	/** 攻撃間隔（秒）。2.0なら2秒に1回攻撃 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackSpeed = 2.0f;

protected:
	/** 最後に攻撃した時刻を記録する変数 */
	double LastAttackTime = 0.0;

	// 連続でオートアタックした回数
		UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Special")
	int32 ConsecutiveAttackCount = 0;

	// 各特殊技の最後に使った時間を記憶（インデックス番号, 時刻）
	TMap<int32, double> SpecialAttackCooldowns;

	// オートアタックの代わりに特殊技を発動できるかチェックする関数
	bool TryUseSpecialAttack();

public: // ★ public に変更（どこからでも呼べるようにする）

	/** 実際に攻撃を実行する関数 */
	// UFUNCTIONを追加して、BTT_Attackから呼べるようにする
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformAutoAttack();	

	/** 攻撃間隔をチェックして攻撃を試行する関数（新しく追加） */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool TryPerformAutoAttack(); // ここに追加	

	/** ダメージ計算と適用を行う関数（重要！） */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OnAttackHit(); // ★この1行が絶対に必要です！

public:

	/** 攻撃時に再生するアニメーション（モンタージュ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* AttackMontage;

	// ダメージを受けた時ののけぞりアニメーション
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* HitReactMontage;

	// 抜刀・納刀のアニメーション
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* UnsheatheMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* SheatheMontage;

public:
	/** リンクする範囲（半径）。FF11なら1000〜1500くらいが目安 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float LinkRadius = 1000.0f;

	/** アクティブな敵かどうか（Trueなら見つけ次第攻撃、Falseなら攻撃されるまで襲ってこないノンアクティブ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	bool bIsActiveEnemy = true;

	/** 周囲の仲間が攻撃された時にリンク（加勢）するかどうか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	bool bCanLink = true;

	/** 周囲の仲間にターゲットを通知する関数 */
	void NotifyNearbyAllies(AActor* TargetToAttack);

	
public:
	// --- 追加：ジョブシステム ---
	// クラスを二重に定義せず、変数と関数だけを追加します

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Stats|Job")
	FDataTableRowHandle JobRow;

	UFUNCTION(BlueprintCallable, Category = "RPG Stats|Job")
	void ApplyJobData();

	// --- 冒険者クラスの等級（ギルドNPCへの申請で昇格） ---

	/** 等級ごとの昇格条件・昇格ボーナスを定義したデータテーブル（行名は昇格先のEAdventurerRank名と一致させる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Stats|Rank")
	UDataTable* AdventurerRankDataTable;

	/** 等級が変化した時にUIへ知らせる合図 */
	UPROPERTY(BlueprintAssignable, Category = "RPG Stats|Rank")
	FOnAdventurerRankChangedSignature OnAdventurerRankChangedDelegate;

	/** 現在の等級の表示名（「五等」「番外」など）をUI表示用に取得する */
	UFUNCTION(BlueprintPure, Category = "RPG Stats|Rank")
	FText GetAdventurerRankDisplayName() const;

protected:
	/** bUsesRestrainedAnimBlueprintの状態に応じて、拘束用ABPへの差し替え／元のABPへの復元を実際に行う */
	void ApplyRestrainedAnimBlueprint();

	/** ターゲットが死亡した際に、次のターゲットを自動で探す関数 */
	void HandleTargetDeath();

	/** 自分を狙っている、または一番近い敵を探す内部関数 */
	AActor* FindBestNextTarget();

public:
#if WITH_EDITOR
	// エディタでの値変更を検知する関数
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	class UInventoryComponent* InventoryComp;

	/** 傷・タトゥー・化粧をインベントリ（箱）形式で一括管理するコンポーネント */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skin Overlay", meta = (AllowPrivateAccess = "true"))
	class USkinOverlayComponent* SkinOverlayComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	class UQuestComponent* QuestComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialog", meta = (AllowPrivateAccess = "true"))
	class UDialogComponent* DialogComp;

	
	// BGM管理コンポーネント
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	class UMusicControlComponent* MusicComp;


};
