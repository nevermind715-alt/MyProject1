// NPCSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Character.h" // キャラクターの構造体を使うためにインクルード
#include "NPCSpawner.generated.h"

USTRUCT(BlueprintType)
struct FStatBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxHP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AttackPower = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DefensePower = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 STR = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 VIT = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DEX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AGI = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Accuracy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Evasion = 0;
		
};

UCLASS()
class MYPROJECT1_API ANPCSpawner : public AActor
{
	GENERATED_BODY()

public:
	ANPCSpawner();

protected:
	virtual void BeginPlay() override;

public:
	// --- スポーン設定 ---

	/** スポーンさせる敵のブループリントクラス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	TSubclassOf<AMyProject1Character> EnemyClass;

	/** リスポーンまでの時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	float RespawnTime = 10.0f;

	//ゲーム開始から最初にスポーンするまでの時間（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	float InitialSpawnDelay = 5.0f;

	/** スポーンできる回数の上限（初回スポーン＋リスポーンの合計）。0にすると無制限にリスポーンします。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxSpawnCount = 0;

	/** 現在までにスポーンした回数（実行中のみ更新される確認用の値です） */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Spawner Settings", Transient)
	int32 CurrentSpawnCount = 0;

	/** 空欄でなければ、プレイヤーがこのフラグを獲得した時に1体だけスポーンする。
	    フラグが外れると再武装し、次にそのフラグが立った時にまた1体スポーンする（周回クエスト用）。
	    設定時はゲーム開始時の自動スポーン（InitialSpawnDelay）と、討伐後の自動リスポーンは行わない */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	FName RequiredFlag;

	/** RequiredFlag運用時のみ有効。フラグを獲得（または別レベルからの復元で検知）してから、敵がスポーンするまでの
	    待機秒数。0なら即時スポーン（従来動作）。ワープで入る闘技場など、入場の数秒後に敵を出したい時に使う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (ClampMin = "0.0"))
	float FlagSpawnDelaySeconds = 0.0f;

	/** 空欄でなければ、このスポナーが湧かせた個体が討伐された時にプレイヤーへ付与するフラグ。
	    お使い・会話（Delivery）クエストなど TargetID で討伐を追えないクエストで、
	    「悪党を倒したか」を依頼主への報告選択肢の RequiredFlag でゲートするために使う。
	    RequiredFlag 運用時は、そのフラグが外れる（＝次の周回が始まる）タイミングでこのフラグも自動で外れる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	FName GrantFlagOnEnemyDeath;

	/** このスポナーが湧かせた個体を討伐した時に、プレイヤーへ渡す賞金（￥）。0なら何も渡さない（従来動作）。
	    討伐の瞬間に所持金へ加算し、ログに「賞金として ○○￥ 手に入れた。」と表示する（闘技場の賞金など） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (ClampMin = "0"))
	int32 GilRewardOnEnemyDeath = 0;

	/** 空欄でなければ、このスポナーが湧かせた個体を討伐した時にプレイヤーへ付与するフラグ。
	    GrantFlagOnEnemyDeathと違い、次の周回（RequiredFlagの消去）でも外さない永続フラグ。
	    すでに持っているフラグは再付与されない（AddFlagの仕様）ので、実質「最初に勝った1回だけ」付与される。
	    「初勝利で上位の試合の選択肢が出る」といった会話側のRequiredFlag等に使う。
	    複数のスポナーで同じ名前にすると「どの相手でも初勝利」、別々の名前にすると「その相手への初勝利」になる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	FName FirstVictoryFlag;

	/** 空欄でなければ、このスポナーが湧かせた個体をプレイヤーが討伐した時、VictoryWarpDelaySeconds秒後に
	    このWarpID（DT_WarpDestinationsの行名）へプレイヤーをワープさせる（闘技場の勝利後の退場など）。
	    RequiredFlag運用時は、ワープの暗転に相乗りしてRequiredFlagも消去する（敗北時と同じく、次の周回の再武装と
	    RequiredFlag連動の表示切替が元に戻る）。待つ間にプレイヤーが倒れた場合は何もしない（敗北側の処理に任せる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	FName VictoryWarpID;

	/** 空欄でなければ、プレイヤーがこのスポナーの試合から退場する時（勝利のVictoryWarpIDへのワープ、または敗北）に、
	    RequiredFlagと一緒にこのフラグも外す。通常／高額など複数のスポナーが同じリングを共有し、RequiredFlagは試合ごとに
	    別にしつつ、見せ用NPCのVisibilityFlag（1つしか持てない）だけ共通にしたい時に使う（共通フラグをここへ指定する）。
	    勝利時は、RequiredFlagを先に（暗転なしで）外し、こちらをワープの暗転に相乗りして外す（見せ用の再表示が暗転の裏になる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings")
	FName FlagToRemoveOnExit;

	/** 討伐してからVictoryWarpIDへワープを始めるまでの待機秒数（勝利の余韻用）。0なら討伐の瞬間に開始する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Settings", meta = (ClampMin = "0.0"))
	float VictoryWarpDelaySeconds = 3.0f;

	// --- 上書き用ステータス（ここで設定した値が敵にコピーされます） ---

	/** 敵のジョブデータ（戦士、モンクなど） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides")
	FDataTableRowHandle SpawnerJobRow;

	// --- NM（レア敵）抽選ポップ設定 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|NM")
	bool bEnableRareSpawn = false;

	/** NMが湧く確率（%） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|NM")
	float RareSpawnChance = 5.0f;

	/** NM湧きに当選した時に使うジョブデータ（データテーブルの行） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|NM", meta = (EditCondition = "bEnableRareSpawn"))
	FDataTableRowHandle RareJobRow;

	/** NMの名前 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|NM", meta = (EditCondition = "bEnableRareSpawn"))
	FString RareNPCName = TEXT("NM Name");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|NM", meta = (EditCondition = "bEnableRareSpawn"))
	FVector RareScale = FVector(1.5f, 1.5f, 1.5f);

	/** スポーンする敵のレベル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Base Stats")
	int32 SpawnerLevel = 1;

	/** trueなら、スポーンする敵のレベルをSpawnerLevelの代わりに「プレイヤーのレベル＋PlayerLevelOffsetMin〜Max（両端を含む）の
	    乱数」にする。スポーンのたびに抽選する。プレイヤーがまだ取得できない時（ゲーム開始直後の自動スポーン等）はSpawnerLevelを使う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Base Stats")
	bool bScaleLevelToPlayer = false;

	/** bScaleLevelToPlayer時の、プレイヤーレベルに足す値の下限（負の値も可） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Base Stats", meta = (EditCondition = "bScaleLevelToPlayer"))
	int32 PlayerLevelOffsetMin = 2;

	/** bScaleLevelToPlayer時の、プレイヤーレベルに足す値の上限。下限より小さい場合は入れ替えて扱う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Base Stats", meta = (EditCondition = "bScaleLevelToPlayer"))
	int32 PlayerLevelOffsetMax = 7;

	//FName から FString に変更しました！
	/** スポーンする敵の名前 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Base Stats")
	FString SpawnerNPCName = TEXT("Goblin");

	//ステータス補正値（ここで設定した数値がプラス・マイナスされます）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Stat Bonus")
	FStatBonus SpawnerStatBonus;

	// --- AI / パトロール設定の上書き ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bOverridePatrolSettings = true;

	// このスポーナーから出る敵はパトロールするか？
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bSpawnerCanPatrol = true;

	/** このスポーナーから出る敵はアクティブ（見つけ次第攻撃）か？ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bSpawnerIsActiveEnemy = true;

	/** このスポーナーから出る敵はリンク（加勢）するか？ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bSpawnerCanLink = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	float SpawnerPatrolRadius = 1000.0f;

	/** trueなら、スポーンした瞬間にプレイヤーをターゲットとして捕捉し、すぐ攻撃行動に入る（BOSS用）。
	    視界・視野角・知覚距離に関係なく捕捉する。bOverridePatrolSettingsの設定とは独立して動作する。
	    一度捕捉した後に見失う挙動は、敵側のbNeverLoseSight（Combat|Sensors）に従う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI")
	bool bAggroPlayerOnSpawn = false;

	// --- 追加：AIの知覚（目と耳）設定 ---

	/** 敵を見つける距離（元の SpawnerPerceptionRadius） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerSightRadius = 1500.0f;

	/** 敵を見失う距離 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerLoseSightRadius = 2000.0f;

	/** 視野角（90で前方180度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerVisionAngle = 90.0f;

	/** 聴覚を有効にするか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	bool bSpawnerEnableHearing = true;

	/** Trueにすると、ジョブ本来のドロップ（データテーブル）を無視してこのスポーナーの設定だけにする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Loot")
	bool bOverrideBaseLoot = false;

	/** この場所で湧いた敵が特別に落とすアイテムリスト */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Loot")
	TArray<FLootItem> SpawnerLootTable;

	/** 聴覚の範囲 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|AI Sensors")
	float SpawnerHearingRange = 3000.0f;

	/** 空欄でなければ、スポーンした敵のUEventDistributorComponent（設定されていれば）のEventPoolIDをこの値で上書きする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event")
	FName SpawnerEventPoolID;

	/** trueの場合、スポーンした敵自身のジョブメッシュ（DT_Jobs::FJobAttributes::CharacterMesh。NM抽選時はRareJobRow側）を
	 *  UEventDistributorComponent::ExtraParticipantMeshOverrideDefaultへ自動設定する。これにより、この敵が
	 *  戦闘敗北で発動したAnimEventのFAnimEventPairing（Extra参加者）に、個別設定の代わりにこの敵自身の見た目を使わせられる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event")
	bool bOverrideExtraParticipantMeshWithOwnMesh = false;

	/** trueの場合、この敵が倒したプレイヤーのイベントでAnimEventが再生されている間、敵自身を非表示にする
	 *  （ExtraPairingsのAAnimEventActorと重なって見えるのを防ぐ。再生終了で自動的に再表示される）。
	 *  スポーンした敵のUEventDistributorComponent::bHideOwnerDuringAnimEventDefaultへ上書きされる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event")
	bool bHideEnemyDuringAnimEvent = false;

	/** 空欄でなければ、このスポナーが湧かせた敵に倒された際、EventDistributorChance(%)の抽選に外れた場合は
	 *  イベントディストリビュータを起動せず、このWarpID（DT_WarpDestinationsの行名。町の治療院など）へ
	 *  ワープしてリスタートさせる。スポーンした敵のUEventDistributorComponent::WarpToRestartIDへ上書きされる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event")
	FName WarpToRestartID;

	/** 設定すると、このスポナーが湧かせた敵に倒された際、イベント抽選より先にこの会話（DT_Dialogsの行）を始める。
	 *  会話の相手は倒した敵。最初の行をShowTextDuringFade（暗転セリフ）にし、そのNextDialogIDの行をActionType=TriggerEvent
	 *  （ActionPayload空欄ならSpawnerEventPoolID）にしておくと、暗転セリフの後にイベント抽選へ進む。
	 *  プレイヤーの全快と敗北時クリーンアップ（Defeat Cleanup）は、その行でイベント抽選が呼ばれた時点で行う。
	 *  WarpToRestartIDの抽選に外れてワープリスタートになる場合は使われない。
	 *  スポーンした敵のUEventDistributorComponent::DefeatDialogRowへ上書きされる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event")
	FDataTableRowHandle DefeatDialogRow;

	/** イベントディストリビュータの抽選に進むためのプレイヤーのステータス条件（例：欲求不満度が50以上）。
	 *  条件を満たさないと抽選自体を行わず、WarpToRestartIDが設定されていれば抽選に外れた時と同じくそこへワープして
	 *  リスタートし、空欄ならEventPoolID未設定の敵に倒された時と同じ通常の敗北復帰（PlayerStartへ復帰）になる。
	 *  条件オフ（初期状態）なら常に抽選へ進む（従来動作のまま）。
	 *  スポーンした敵のUEventDistributorComponent::EventDistributorStatConditionへ上書きされる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event")
	FEventStatCondition EventDistributorStatCondition;

	/** イベントディストリビュータを起動する確率（%）。抽選に外れると、WarpToRestartIDが設定されていればそこへワープして
	 *  リスタートし、空欄なら通常の敗北復帰（PlayerStartへ復帰）になる。初期値100＝常にイベントディストリビュータを
	 *  起動する（従来動作のまま）。スポーンした敵のUEventDistributorComponent::EventDistributorChanceへ上書きされる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float EventDistributorChance = 100.0f;

	// --- 敗北時クリーンアップ（この敵にプレイヤーが倒され、イベント発動またはワープリスタートが行われた直後に実行） ---

	/** trueなら、プレイヤーを倒したこのスポナー産の敵を、イベント発動の直後に削除する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event|Defeat Cleanup")
	bool bRemoveOnPlayerDefeat = false;

	/** 空欄でなければ、敗北時にこのQuestIDのクエストを失敗扱い（CancelQuestと同じ後始末＋「依頼を失敗した…」ログ＋
	 *  DT_QuestDataのFailurePenalty適用。再受注可）にする。
	 *  クエストのAcceptFlagも外すので、それをRequiredFlagにしているQuestItemPointは自動的に非表示になる。
	 *  進行中でなければクエスト自体は何も起きない */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Overrides|Event|Defeat Cleanup")
	FName FailQuestIDOnPlayerDefeat;

	/** 敗北時に削除するレベル上のActor（この敵を湧かせるフラグを立てるQuestItemPoint等。同じレベルに置いたものを指定する）。
	 *  別レベルから戻った場合は再び配置されるため、永続的に消したいポイントはRequiredFlagで隠す運用と併用すること */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spawner Overrides|Event|Defeat Cleanup")
	TArray<TObjectPtr<AActor>> ActorsToRemoveOnPlayerDefeat;

protected:
	// --- 内部処理 ---

	/** 実際にスポーンさせる関数 */
	void SpawnEnemy();

	/** 敵が死んだ時に呼ばれる関数 */
	UFUNCTION()
	void OnEnemyDeath(AActor* DeadActor);

	/** RequiredFlag運用時、今のフラグ周回で既にスポーン済み（FlagSpawnDelaySeconds待機中も含む）かどうか */
	bool bSpawnedForCurrentFlag = false;

	/** RequiredFlag運用時のスポーン開始。FlagSpawnDelaySecondsが0より大きければその秒数後にSpawnEnemyする */
	void StartFlagSpawn();

	/** FlagSpawnDelaySeconds待機中のタイマー。待機中にフラグが外れたらキャンセルする */
	FTimerHandle FlagSpawnTimerHandle;

	/** 討伐後、VictoryWarpDelaySeconds秒待ってから呼ばれる。VictoryWarpIDへワープし、RequiredFlagも消去する */
	void ExecuteVictoryWarp();
	FTimerHandle VictoryWarpTimerHandle;

	/** この敵のUEventDistributorComponent::OnDefeatedPlayerの購読先。bRemoveOnPlayerDefeat等に従って
	    クエスト失敗・RequiredFlag解除・指定Actor／敵の削除を行う（UFUNCTION不要のC++専用デリゲート） */
	void OnEnemyDefeatedPlayer();

	/** 敗北後の敵削除。イベント進行中（bHasActiveEvent）は敵がEventContextActorとして使われるため、終了まで待ってから消す */
	void RemoveEnemyAfterEvent();
	FTimerHandle RemoveEnemyTimerHandle;

	/** プレイヤーがフラグを獲得した時に呼ばれる（OnFlagAddedの購読先）。RequiredFlag一致時のみスポーンする */
	UFUNCTION()
	void OnPlayerFlagAdded(FName FlagName);

	/** プレイヤーがフラグを失った時に呼ばれる（OnFlagRemovedの購読先）。RequiredFlag一致時、次周のため再武装する */
	UFUNCTION()
	void OnPlayerFlagRemoved(FName FlagName);

	/** RequiredFlag運用時の初期化。プレイヤー生成と、別レベルからのステータス復元(ApplyPendingCharacterLoad)を
	    待ってからRequiredFlagを評価する。復元はAddFlagを経由せずUnlockedFlagsを一括代入するためOnFlagAddedが
	    飛ばず、BeginPlay一度きりの判定ではレベル遷移後に取りこぼす。成立するまで短間隔でリトライする */
	void InitFlagSpawnWhenReady();

	/** RequiredFlag / GrantFlagOnEnemyDeath の状態から、今このスポナーが1体スポーンすべきか判定する */
	bool ShouldSpawnForFlagState(const class AMyProject1Character* PlayerChar) const;

	/** InitFlagSpawnWhenReadyのリトライ用タイマー・試行回数、および購読済みフラグ */
	FTimerHandle FlagInitTimerHandle;
	int32 FlagInitAttempts = 0;
	bool bFlagDelegatesBound = false;

	/** リスポーンタイマー */
	FTimerHandle RespawnTimerHandle;

	/** 現在スポーンしている敵の参照 */
	UPROPERTY()
	AMyProject1Character* SpawnedEnemy;

#if WITH_EDITORONLY_DATA
	/** エディタ上で場所がわかるようにするためのアイコン */
	UPROPERTY()
	class UBillboardComponent* SpriteComponent;

	/** 向いている方向を表示する矢印 */
	UPROPERTY()
	class UArrowComponent* ArrowComponent;
#endif
};