// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Kismet/KismetMathLibrary.h" // これを追加
#include "MyProject1Types.h" // これを追加！
#include "InventoryComponent.h"
#include "InputActionValue.h"
#include "MyProject1Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChangedSignature, float, CurrentHP, float, MaxHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLogScrollToBottomSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeathSignature, AActor*, DeadActor);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AMyProject1Character : public ACharacter
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

	/** ズーム操作用の入力アクション（Enhanced Input用） */
	UPROPERTY(EditAnywhere, Category = "Input")
	class UInputAction* ZoomAction;

	/** 非戦闘時のHP自動回復の間隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Regeneration")
	float AutoRecoveryInterval = 5.0f;

	/** 1回あたりの回復量（最大HPに対する割合 0.05 = 5%） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Regeneration")
	float AutoRecoveryRate = 0.05f;

	/** 自動回復を管理するタイマーハンドル */
	FTimerHandle TimerHandle_AutoRecovery;

	/** 定期的に呼ばれる回復関数 */
	void HandleAutoRecovery();

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

	/** 入力をフックするための関数 */
	void HandleJumpCompleted();

	/** カメラの最短距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MinTargetArmLength = 200.0f;

	/** カメラの最長距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MaxTargetArmLength = 1000.0f;

	/** 1回のホイール操作で動く距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ZoomStep = 50.0f;

	/** ズーム入力を処理する関数 */
	void ZoomCamera(const struct FInputActionValue& Value);

	void ExpireItemBuff(FString ItemName, TArray<FItemEffect> Effects);

	
	

public:
	/** Constructor */
	AMyProject1Character();

	// ★追加：キャラクターの名前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FString CharacterName = "Senshi";

	// ★追加：UIに名前を通知するイベント
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateHealthWidgetName(const FString& NewName);

	/** 武器を表示するためのコンポーネント（宣言のみにする） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMeshComp;

	/** 操作をロックするかどうかのフラグ */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
	bool bIsInputLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleMenuAction;

	/** 操作ロックを切り替える関数（BPから呼び出し可能） */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetInputLocked(bool bLocked);

	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	bool IsReadingOldLogs() const;

	/** NPCとしてログに台詞を表示する */
	UFUNCTION(BlueprintCallable, Category = "Combat|UI")
	void TalkToLog(const FString& Message);

	// 2. WidgetからBindできるように UPROPERTY(BlueprintAssignable) をつける
	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnHPChangedSignature OnHPChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Combat|UI")
	FOnLogScrollToBottomSignature OnLogScrollToBottomDelegate;

	/** HPを増減させる汎用関数（正の数で回復、負の数でダメージ） */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float UpdateHealth(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ToggleCombatMode();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Buff")
	void ApplyItemBuff(FString ItemName, const TArray<FItemEffect>& Effects, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Character|Audio")
	void PlayFootstepSound();

	/** 材質ごとの音を登録するマップ（データテーブルでも可） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Audio")
	TMap<TEnumAsByte<EPhysicalSurface>, USoundBase*> FootstepSounds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Audio")
	class USoundAttenuation* FootstepAttenuation;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnCharacterDeathSignature OnDeathDelegate;

protected:

	void OnToggleMenuPressed();

	/** インプットのセットアップ */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** ゲーム開始時に呼ばれる */
	virtual void BeginPlay() override;

	
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
	void OnReceiveLogMessage(const FString& Message, ELogMessageType InLogType);

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
	void SetCurrentTarget(AActor* NewTarget);

	// ★追加：ターゲットを明示的に解除する関数
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CancelTarget();

	// ★追加2：BP側でマーカーの表示/非表示を制御するためのイベント
	// C++から呼び出して、BP側に処理を投げます
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnTargetUpdated(AActor* TargetActor, bool bIsTargeted);

	/** ターゲットできる最大距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float TargetingRange = 2000.0f;

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
	bool IsDead() const { return bIsDead; }

protected:
	// 死亡処理
	virtual void OnDeath();

	// 死亡フラグ
	bool bIsDead = false;

public:
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

	/** ★追加: 攻撃ミス時の共通SE */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Sounds")
	USoundBase* GlobalMissSound;

	/** 攻撃間隔（秒）。2.0なら2秒に1回攻撃 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackSpeed = 2.0f;

protected:
	/** 最後に攻撃した時刻を記録する変数 */
	double LastAttackTime = 0.0;

public: // ★ public に変更（どこからでも呼べるようにする）

	/** 実際に攻撃を実行する関数 */
	// ★ UFUNCTIONを追加して、BTT_Attackから呼べるようにする
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

	// ★ 追加：抜刀・納刀のアニメーション
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* UnsheatheMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* SheatheMontage;

public:
	/** リンクする範囲（半径）。FF11なら1000〜1500くらいが目安 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|AI")
	float LinkRadius = 1500.0f;

	/** 周囲の仲間にターゲットを通知する関数 */
	void NotifyNearbyAllies(AActor* TargetToAttack);

	
public:
	// --- 追加：ジョブシステム ---
	// クラスを二重に定義せず、変数と関数だけを追加します

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Stats|Job")
	FDataTableRowHandle JobRow;

	UFUNCTION(BlueprintCallable, Category = "RPG Stats|Job")
	void ApplyJobData();

protected:
	/** ターゲットが死亡した際に、次のターゲットを自動で探す関数 */
	void HandleTargetDeath();

	/** 自分を狙っている、または一番近い敵を探す内部関数 */
	AActor* FindBestNextTarget();

protected:
#if WITH_EDITOR
	// エディタでの値変更を検知する関数
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	class UInventoryComponent* InventoryComp;

};