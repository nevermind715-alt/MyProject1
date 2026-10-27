#pragma once

#include "CoreMinimal.h"
#include "MyProject1Character.h"
#include "MyProject1Types.h"
#include "QuestNPCBase.generated.h"

// クエストを提示するNPCの共通C++基底クラス。AMyProject1Characterを継承することで、
// 移動（AMyAIController）・アニメーション（UMyProject1AnimInstanceBase）・装備（Hair/Face含む全メッシュスロット）の
// 仕組みをそのまま流用できる（これらはコンポーネント化されておらずAMyProject1Character本体に組み込まれているため、
// 継承せずに一部だけ切り出すと結局同じ仕組みを複製することになり非効率）。
// 戦闘機能自体は使わない想定だが、bIsActiveEnemy=falseにしておけばAMyAIControllerが攻撃状態に移行しないため、
// 実害はない。
// 複数のFQuestDialogSetを優先順位付きで持てるので、1体のNPCが「クエスト1完了後は自動でクエスト2を提示する」
// といった連鎖・分岐を、Blueprint側に判定ロジックを書かずに表現できる。
UCLASS()
class MYPROJECT1_API AQuestNPCBase : public AMyProject1Character
{
	GENERATED_BODY()

public:
	AQuestNPCBase();

protected:
	virtual void BeginPlay() override;

	// 向き直り中にNPCが破棄された場合（ワープ等）に、タイマー解除とプレイヤーの入力ロック解除を行う
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ログ表示等に使うNPCの名前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	FString NPCName;

	// このNPCが使う会話データテーブル（DT_Dialogs）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	class UDataTable* DialogTable;

	// このNPCが扱うクエストのリスト。配列の先頭ほど優先度が高い。
	// 進行中/報告待ちのクエストがあればそれを継続表示し、無ければ受注条件を満たした先頭の未受注クエストを提示する
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	TArray<FQuestDialogSet> Quests;

	// --- 初回だけの挨拶（クエストと無関係な一度きりの状態変化用） ---
	// 空欄なら使わない（従来通りQuests配列のみで判定）。設定すると、このフラグを未取得の間だけ
	// DialogRowName_FirstMeetを優先表示する。フラグの付与はDT_Dialogs側の対象行のGrantFlagで行う想定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|FirstMeet")
	FName FirstMeetFlag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|FirstMeet")
	FName DialogRowName_FirstMeet;

	// --- 条件（フラグ／代行者等級）に応じた会話の出し分け ---
	// 空配列なら使わない（従来通りFirstMeetFlag→Quests配列のみで判定）。
	// 設定すると、FirstMeetFlagの判定後・Quests配列の判定前にこの配列を先頭から評価し、
	// 条件を満たした最初のエントリの行を表示する（FirstMeetと違い一度きりではなく、話しかけるたびに現在の状態の行を出す）。
	// どのエントリにも一致しなければ、そのままQuests配列の判定に進む（末尾に無条件エントリを置けば既定の台詞になる）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Conditional Dialog")
	TArray<FConditionalDialogEntry> ConditionalDialogs;

	// --- 日替わり会話シーケンス（多日クエストの自動進行） ---
	// チェックを入れると、このNPCは DailyDialogSequenceRows を「ゲーム内で日付が変わるごとに1行ずつ」順番に表示する。
	// 進行状況（何行目まで進んだか／最後に進めた日）はプレイヤーのセーブデータに自動記録されるため、
	// GrantFlag・ExcludeFlag・NextDialogIDチェーンの手設定は不要。判定はFirstMeetFlagの後・ConditionalDialogsの前。
	// 各行の GrantFlag / ActionType(AddItem等) はそのまま使えるので、アイテム受け渡しは行データ側で行う
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Daily Sequence")
	bool bEnableDailyDialogSequence = false;

	// 進行記録を保存するための一意なID（NPCごとに別名にする。例 "NPC_A"）。
	// これがセーブ内のキーになるので、後から変更すると進行がリセットされる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Daily Sequence", meta = (EditCondition = "bEnableDailyDialogSequence"))
	FName DailyDialogSequenceID;

	// 1日1ステップずつ表示するシーケンス。先頭が初回、以降は日をまたぐごとに次へ進む。
	// 各要素はConditionalDialogsと同じ条件セット（等級・数値ステータス・所持アイテム）を個別に持てる。
	// 条件未達の日はそのステップを飛ばさず、達成するまで同じステップのまま待つ（次の日にまた判定し直す）。
	// ＋ボタンで末尾に追加していくだけでよい（並べ替え不要）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Daily Sequence", meta = (EditCondition = "bEnableDailyDialogSequence"))
	TArray<FDailyDialogSequenceStep> DailyDialogSequenceRows;

	// その日はもう進めた時に表示する行（空欄なら通常の会話判定へフォールスルー）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Daily Sequence", meta = (EditCondition = "bEnableDailyDialogSequence"))
	FName DailyDialogSequenceWaitRow;

	// 全行を表示し終えた後に表示する行（空欄なら通常の会話判定へフォールスルー）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Daily Sequence", meta = (EditCondition = "bEnableDailyDialogSequence"))
	FName DailyDialogSequenceDoneRow;

	// --- 1日1回だけ話せる会話 ---
	// チェックを入れると、ゲーム内で1日（日付が変わるまで）に1回だけ DialogRowName_OncePerDay を表示し、
	// 同じ日にもう一度話しかけた場合は DialogRowName_OncePerDayDone を表示する。日付が変わると自動でまた話せる。
	// 「最後に話した日」を会話開始時にプレイヤーのセーブデータへ記録するので、フラグの手管理は不要。
	// 判定はFirstMeetFlagと日替わりシーケンスの後・ConditionalDialogsの前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Once Per Day")
	bool bEnableOncePerDayDialog = false;

	// 記録を保存するための一意なID（NPCごとに別名にする。DailyDialogSequenceIDとも別名にすること）。
	// これがセーブ内のキーになるので、後から変更すると記録がリセットされる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Once Per Day", meta = (EditCondition = "bEnableOncePerDayDialog"))
	FName OncePerDayID;

	// その日の最初の会話で表示する行
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Once Per Day", meta = (EditCondition = "bEnableOncePerDayDialog"))
	FName DialogRowName_OncePerDay;

	// その日すでに話した後に表示する行（空欄／DialogTableに無い場合は通常の会話判定へフォールスルー）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Once Per Day", meta = (EditCondition = "bEnableOncePerDayDialog"))
	FName DialogRowName_OncePerDayDone;

	// 日替わり会話シーケンスを1歩進める（会話行の到達だけでは進めない。DialogのbAdvanceDailySequenceがtrueの
	// 選択肢／行から呼ばれる）。「はい/いいえ」のような分岐で、実際にステップを完了したと判断できる方にだけ
	// チェックを入れることで、「いいえ」を選んだ場合は進行が更新されず、次に話しかけた時に同じ行がもう一度出る、
	// という制御ができる
	UFUNCTION(BlueprintCallable, Category = "QuestNPC|Daily Sequence")
	void AdvanceDailyDialogSequence(class AMyProject1Character* PlayerChar);

	// 会話開始時にプレイヤーの方へ体（ヨーのみ）を向け、会話終了時に元の向きへ戻す。
	// 座っているNPCなど向きを固定したい個体はチェックを入れる（チェックすると向き直らない）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	bool bDoNotTurnToPlayerOnTalk = false;

	// プレイヤーへ向き直る回転速度（度/秒）。一定速度で回すため、180度なら 360 で0.5秒・180 で1秒かかる。
	// 向き直りが完了してから会話ウィンドウを開く
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC", meta = (ClampMin = "1.0"))
	float TurnToPlayerSpeedDegPerSec = 360.0f;

	// BPI_InteractableのイベントからEキー等で呼び出す、会話開始の共通ネイティブ処理
	UFUNCTION(BlueprintCallable, Category = "QuestNPC")
	void TalkToNPC(class AMyProject1Character* PlayerChar);

	// --- フラグによる表示/非表示切り替え ---
	// 例：「元の場所のNPC」にHideOnFlag、「移動先に先置きした同じNPC」にShowOnFlagを
	// 同じフラグ名で設定しておくと、会話でフラグが立った瞬間に入れ替わったように見える
	/** 表示切り替えの判定に使うフラグ（空欄、またはVisibilityModeがNoneなら常に表示のまま） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Flag Visibility")
	FName VisibilityFlag;

	/** VisibilityFlagの状態に応じて、表示するか消えるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Flag Visibility")
	EFlagVisibilityMode VisibilityMode = EFlagVisibilityMode::None;

	/** ゲーム内時間帯による表示切り替え（例：18時〜22時だけ居るNPC）。VisibilityFlagと併用した場合は、
	    フラグ条件と時間帯の両方を満たしている間だけ表示される */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Time Visibility")
	FActiveTimeWindow ActiveTimeWindow;

	// --- NPC装備（見た目のみ） ---
	// DT_Equipments（EquipmentDataTable、AMyProject1Character由来）のRowNameを並べるだけで、
	// BeginPlay時に継承済みのEquipItem()経由でスケルタルメッシュ／スタティックメッシュ／スキンオーバーレイの
	// 全てを反映できる。個別メッシュを直接指定する必要がなくなり、DataTable側の編集だけで見た目を差し替えられる。
	// StatModifiers（ステータス補正）はShouldApplyEquipmentStatBonuses()のオーバーライドにより無視されるため、
	// ここに指定した装備はMyStatsに一切影響しない（純粋に見た目だけ）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Equipment")
	TArray<FName> InitialEquipmentRowNames;

	// DT_Equipmentsと同じ考え方で、SkinOverlayComp側の刺青・傷跡もRow名を並べるだけで初期化できるようにする
	// （SkinOverlayComp->TattooDataTable / ScarDataTableのRowName。テーブル自体はBP DetailsのSkinOverlayCompで設定する）。
	// AddOverlay()経由で反映されるが、これもShouldApplyEquipmentStatBonuses()により見た目だけでMyStatsは変化しない
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Equipment")
	TArray<FName> InitialTattooOverlayRowNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Equipment")
	TArray<FName> InitialScarOverlayRowNames;

	// --- 常時アニメーション再生（街中で踊るNPC等） ---
	// DT_AnimSequences（UMyProject1GameInstance::AnimSequenceDataTable）の行名を指定すると、BeginPlay時にその行の
	// Montageをこのキャラ自身のAnimInstanceでループ再生し続ける（空欄なら何もしない）。
	// 行のMeshLocationOffset/MeshRotationOffsetはメッシュの相対位置に加算される。PropMeshが設定されていれば、
	// PropRelativeLocation/RotationのActor相対位置にこのNPCのコンポーネントとして小道具を表示する。
	// Line/Sound/ExtraPairings/bHideAllEquipmentDuringPlay/TemporaryEquipItemIDsは使わない。
	// 再生中はGameInstance側のAnimEvent再生とは独立しているため、複数のNPCが同時に再生できる。
	// 再生中はCharacterMovementをMOVE_Noneにし、AIロジック（BehaviorTree）も止める（RootMotionや徘徊で位置がズレないように）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Idle Animation")
	FName IdleAnimSequenceRowName;

	// --- 表情モーフの上書き（プレイヤーと同じメッシュをNPCに使う場合の表情づくり用） ---
	// キー＝モーフターゲット名、値＝モーフの強さ（0〜1）。BeginPlay時に一度だけUSkeletalMeshComponent::SetMorphTargetで
	// セットする。SetMorphTargetの値はアニメ（ABP）が出すモーフカーブより後に適用されて上書きするため、毎フレームの処理は不要。
	// 0を指定すると「アニメ側が出している値を0に抑える」上書きになる。
	// BlinkMorphName（まばたきのモーフ）をここに入れた場合は、まばたきが上書きを消してしまわないよう、このNPCの自動まばたきを止める
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QuestNPC|Face Morph")
	TMap<FName, float> FaceMorphOverrides;

	/** 表情モーフの上書きを1つ追加／更新して即座に反映する（実行中に表情を変えたい時用。FaceMorphOverridesにも記録される） */
	UFUNCTION(BlueprintCallable, Category = "QuestNPC|Face Morph")
	void SetFaceMorphOverride(FName MorphTargetName, float Value);

	/** FaceMorphOverridesで上書きしているモーフを全て0へ戻し、上書きを解除する（止めていた自動まばたきも再開する） */
	UFUNCTION(BlueprintCallable, Category = "QuestNPC|Face Morph")
	void ClearFaceMorphOverrides();

	/** 会話終了後の向き戻し（ETurnMode::ReturnAfterTalk）が進行中なら、即座にPreTalkRotationへ確定させて打ち切る。
	    AnimEvent（NPCとの位置合わせアニメーション）等、このNPCのActor向きを外部から使いたい処理が、
	    TickTurnによる戻り回転の途中に基準値をキャッシュしてしまい、位置がズレるのを防ぐために呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "QuestNPC")
	void StopReturnTurnImmediately();

protected:
	// 装備によるステータス補正（StatModifiers）を無効化し、NPCの装備を見た目だけの変化にする
	virtual bool ShouldApplyEquipmentStatBonuses() const override { return false; }

private:
	/** FaceMorphOverridesの全エントリをメッシュへ反映する（BeginPlayから呼ぶ） */
	void ApplyFaceMorphOverrides();

	/** 1エントリ分をメッシュへ反映する共通処理（BlinkMorphNameなら自動まばたきも止める） */
	void ApplyFaceMorph(FName MorphTargetName, float Value);

	/** InitialEquipmentRowNamesをEquipmentDataTableから引いて、BeginPlay時に見た目だけ反映する */
	void ApplyInitialEquipment();

	/** IdleAnimSequenceRowNameのDT_AnimSequences行を引いて、Montageのループ再生とメッシュ位置オフセットを開始する */
	void StartIdleAnimSequence();

	/** BeginPlay後、プレイヤー生成と別レベルからのステータス復元(ApplyPendingCharacterLoad)を待ってから
	    VisibilityFlagを評価する。別レベル復元はUnlockedFlagsを一括代入するだけでOnFlagAddedが飛ばないため、
	    復元がこのNPCのBeginPlayより後にずれ込むケースを数回の再チェックで拾う（ANPCSpawnerと同じ方式） */
	void InitFlagVisibilityWhenReady();

	/** VisibilityFlag/VisibilityModeの設定に応じて、現在の表示/当たり判定を更新する */
	void UpdateFlagVisibility(const class AMyProject1Character* PlayerChar);

	/** bHasFlag（フラグを持っているか）を基に表示/当たり判定を反映する共通処理
	    （UpdateFlagVisibility・OnPlayerFlagAdded・OnPlayerFlagRemovedの実体を1箇所にまとめたもの） */
	void ApplyVisibilityForFlagState(bool bHasFlag);

	/** フラグ条件（bFlagAllowsVisible）と時間帯条件（bTimeAllowsVisible）の両方を満たす時だけ表示/当たり判定ONにする */
	void ApplyCombinedVisibility();

	/** ActiveTimeWindowが有効なら、OnInGameTimeChangedを購読して現在時刻で表示を更新する（BeginPlayから呼ぶ） */
	void InitTimeWindowVisibility();

	/** ゲーム内時間が進むたびに呼ばれる（OnInGameTimeChangedの購読先）。時間帯の内外が変わったら表示を更新する */
	UFUNCTION()
	void OnInGameTimeChangedHandler(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute);

	/** プレイヤーがフラグを獲得した時に呼ばれる（OnFlagAddedの購読先）。VisibilityFlag一致時のみ表示を更新する */
	UFUNCTION()
	void OnPlayerFlagAdded(FName FlagName);

	/** プレイヤーがフラグを失った時に呼ばれる（OnFlagRemovedの購読先）。VisibilityFlag一致時のみ表示を更新する。
	    これが無いとRemoveFlagで消したフラグにNPCの表示/非表示が追従せず、部屋の出入りを挟んでも
	    見た目（表示状態）が古いままになる */
	UFUNCTION()
	void OnPlayerFlagRemoved(FName FlagName);

	// --- 会話開始時にプレイヤーへ向き直る／会話終了時に元へ戻る処理 ---

	/** 現在の一定速度回転が何のための回転か。会話前の向き直りと、会話後の戻りで完了時の挙動が変わる */
	enum class ETurnMode : uint8
	{
		None,
		FacePlayerThenTalk, // 向き直り完了後に会話を開始する
		ReturnAfterTalk     // 会話終了後、PreTalkRotationへ戻す（完了後は何もしない）
	};

	/** TalkToNPCで会話行が決まった後の共通入口。必要ならプレイヤーへ向き直ってから会話を開始する。
	    bDoNotTurnToPlayerOnTalkが真、または既にほぼ正対している場合は即座に会話を開始する */
	void StartDialogFacingPlayer(class AMyProject1Character* PlayerChar, class IRpgCharacterInterface* RpgInterface, FName RowName, const FString& LogContext);

	/** 実際にTryStartDialogを呼ぶ。成功時は入力ロック＋OnDialogClosed購読、
	    失敗時はログ出力と、bDidTurn（先行して向き直り・入力ロック済み）なら巻き戻しを行う */
	void BeginDialogRow(class UDialogComponent* PlayerDialogComp, class IRpgCharacterInterface* RpgInterface, FName RowName, const FString& LogContext, bool bDidTurn);

	/** ModeでTurnTimerHandleの一定速度回転を開始する（前回時刻の初期化とタイマー設定をまとめたもの） */
	void StartTurnTimer(ETurnMode Mode);

	/** TurnTimerHandleを停止し、ETurnMode::Noneへ戻す */
	void FinishTurn();

	/** TurnTimerHandleで回るコールバック。TurnToPlayerSpeedDegPerSecの一定速度でヨーを回し、
	    完了したらTurnModeに応じて会話開始（FacePlayerThenTalk）または何もしない（ReturnAfterTalk） */
	void TickTurn();

	/** プレイヤー方向を向くヨーのみの回転を返す（ピッチ/ロールは0） */
	FRotator MakeFacePlayerRotation(const AActor* PlayerChar) const;

	/** 会話終了（UDialogComponent::OnDialogClosed）で呼ばれ、向き直り前の向きへ（同じ一定速度で）戻して購読を解除する */
	UFUNCTION()
	void OnTalkDialogClosed();

	/** 向き直り／戻り中のタイマー */
	FTimerHandle TurnTimerHandle;

	/** 現在のTurnTimerHandleの用途 */
	ETurnMode TurnMode = ETurnMode::None;

	/** ReturnAfterTalk時の回転先（固定値。FacePlayerThenTalk時はプレイヤー位置から毎tick再計算する） */
	FRotator TurnFixedTarget = FRotator::ZeroRotator;

	/** TickTurnで一定速度回転を出すための前回実行時刻（秒） */
	float LastTurnTickTime = 0.0f;

	/** 向き直り完了後に開始する会話の保留情報 */
	TWeakObjectPtr<class AMyProject1Character> PendingPlayerChar;
	TWeakObjectPtr<class UDialogComponent> PendingDialogComp;
	FName PendingDialogRow;
	FString PendingLogContext;

	/** 向き直りで向きを変える直前の回転。会話終了時にここへ戻す */
	FRotator PreTalkRotation = FRotator::ZeroRotator;

	/** PreTalkRotationが有効か（未会話・向き直り無効の個体で誤って戻さないためのガード） */
	bool bHasPreTalkRotation = false;

	/** OnDialogClosedを購読しているプレイヤーのDialogComponent（会話終了時に解除するため保持） */
	TWeakObjectPtr<class UDialogComponent> TalkDialogCompBound;

	/** InitFlagVisibilityWhenReadyの再試行タイマー／試行回数／OnFlagAdded購読済みフラグ */
	FTimerHandle FlagVisibilityInitTimerHandle;
	int32 FlagVisibilityInitAttempts = 0;
	bool bFlagVisibilityDelegatesBound = false;

	/** ApplyCombinedVisibilityが使う、フラグ条件／時間帯条件それぞれの現在の判定結果（未設定の条件は常に真） */
	bool bFlagAllowsVisible = true;
	bool bTimeAllowsVisible = true;
};
