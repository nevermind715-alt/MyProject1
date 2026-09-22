#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "MyProject1Types.h" // ★ワープ先の構造体を使うために追加
#include "MyProject1GameInstance.generated.h"


class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarpFadeOutRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnInGameTimeChanged, int32, Year, int32, Month, int32, Day, int32, Hour, int32, Minute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDayChangedSignature);

// PlayAnimSequenceEventの完了通知。bCompletedNormally=trueは全ステップ再生完了、falseは対象/アセット不備などで開始できなかった場合
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimSequenceEventFinished, bool, bCompletedNormally);

UCLASS()
class MYPROJECT1_API UMyProject1GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "Time")
	FOnInGameTimeChanged OnInGameTimeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Time")
	FOnDayChangedSignature OnDayChangedDelegate;

	// 現在のゲーム内時間を「分」だけで持つ（0〜1439） 例：8時間 * 60 = 480
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Time")
	int32 CurrentTimeInMinutes = 480;

	// 現実時間の何秒で、ゲーム内の1分を進めるか（初期値5.0秒 ＝ 現実2時間でゲーム内1日）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	float RealSecondsPerGameMinute = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Calendar")
	int32 CurrentYear = 2026;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Calendar")
	int32 CurrentMonth = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Calendar")
	int32 CurrentDay = 11;

	
	// （ゲーム内時間が1日進むたびに+1される絶対的なカウンター）
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Time|Calendar")
	int32 TotalElapsedDays = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time|Calendar")
	TArray<FCyclePhaseSettings> CyclePhaseRules;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Time|Calendar")
	ECycleState CurrentCycleState;

	/** 待機/睡眠などで時間を一気に進める。日をまたぐ場合は日数分だけAdvanceDayを個別に呼ぶので、
	 *  日付ベースの仕組み（月齢/CurrentCycleStateやOnDayChangedDelegate依存のクエスト等）も正しく動く。
	 *  UI更新の通知（OnInGameTimeChanged）は進めた後に1回だけ行う。 */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void AdvanceTimeBy(int32 MinutesToAdd);

	// ゲーム開始時に呼ばれる関数（ここでタイマーを動かします）
	virtual void Init() override;

	// --- ワープ設定 ---

	UPROPERTY(BlueprintAssignable, Category = "Warp")
	FOnWarpFadeOutRequested OnWarpFadeOutRequested;

	/** ワープ先名簿（データテーブル）をセットする場所 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	UDataTable* WarpDataTable;

	// --- ワープの記憶領域 ---

	/** 暗転アニメーションの長さ（秒）。WBP_LoadingScreen側の暗転Widgetアニメーションと同じ秒数に合わせること。
	 *  この秒数が経過した時点でExecuteWarpProcess()が自動的に呼ばれる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	float WarpFadeOutDuration = 1.0f;

	/** 明転アニメーションの長さ（秒）。この秒数が経過した時点で入力（EnableInput）が戻る */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	float WarpFadeInDuration = 1.0f;

	/** マップ移動を伴うワープの待機中（ロード中）か？ */
	UPROPERTY(BlueprintReadOnly, Category = "Warp")
	bool bHasPendingWarp = false;

	/** 移動先の座標と向きの記憶 */
	UPROPERTY(BlueprintReadOnly, Category = "Warp")
	FTransform PendingWarpTransform;

	// --- ワープ実行関数 ---

	/** ワープを要求する（同じマップなら即移動、別マップならロードを挟む）
	 *  bBypassRequiredFlag: trueにするとRequiredFlagの所持チェックを無視する（デバッグワープメニュー用） */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void RequestWarp(FName WarpID, class ACharacter* PlayerCharacter, bool bBypassRequiredFlag = false);

	UFUNCTION(BlueprintCallable, Category = "Warp")
	void ExecuteWarpProcess();

	/** エリアChangeと同じ暗転（OnWarpFadeOutRequested）を再利用して、画面が真っ暗になった瞬間にFlagNameを
	 *  TargetCharacterへ付与する。ワープを伴わない「NPCの表示切替を暗転の裏で行いたい」時に使う */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void RequestFadeThenGrantFlag(FName FlagName, class AMyProject1Character* TargetCharacter);

	/** RequestFadeThenGrantFlagの消去版。既にWarp/WallWarpの暗転が同フレームで予約済みなら、
	 *  そちらに相乗りして二重に暗転させず、フラグの消去だけをExecuteWarpProcess側で一緒に反映する */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void RequestFadeThenRemoveFlag(FName FlagName, class AMyProject1Character* TargetCharacter);

	/** 待機/睡眠による時間スキップの前に暗転を挟む要求。エリアChangeと全く同じOnWarpFadeOutRequestedを
	 *  使うので、画面が真っ暗になった瞬間にExecuteWarpProcess側でAdvanceTimeByが実行される。
	 *  bIsSleepがtrueの場合、疲労度は増える代わりに睡眠時間に応じて回復する */
	UFUNCTION(BlueprintCallable, Category = "Time")
	void RequestFadeThenAdvanceTime(int32 MinutesToAdd, class ACharacter* TargetCharacter, bool bIsSleep = false);

	/** AWallWarpLinkからの要求。同一レベル内の軽量ワープにも、エリアChangeと同じ暗転演出を挟む。
	 *  暗転が終わった瞬間にSourceLink->ExecuteWarpNow()を呼び、実際のテレポートを行う */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void RequestFadeThenWallWarp(class AWallWarpLink* SourceLink, class ACharacter* TargetCharacter);

	/** DT_WarpDestinationsの全行を、UI表示用の軽量データ一覧として取得する（デバッグメニュー等がBP側で一覧を組み立てる際に使う） */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	TArray<FWarpDestinationInfo> GetAllWarpDestinations() const;

	/** マップのロード完了後に呼ばれ、記憶した座標にプレイヤーを動かす */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void ApplyPendingWarp(class ACharacter* PlayerCharacter);

	// --- 傷・タトゥー・ピアス・病気の記憶領域 ---
	// レベル移動（OpenLevel）でキャラクターが再生成されても消えないよう、
	// GameInstance側に「箱の中身」のコピーを保持しておく（PendingWarpTransformと同じ仕組み）。

	UPROPERTY(BlueprintReadOnly, Category = "Skin Overlay")
	TMap<FName, FActiveSkinOverlayState> SavedActiveTattoos;

	UPROPERTY(BlueprintReadOnly, Category = "Skin Overlay")
	TMap<FName, FActiveSkinOverlayState> SavedActiveScars;

	UPROPERTY(BlueprintReadOnly, Category = "Skin Overlay")
	TMap<FName, FActiveSkinOverlayState> SavedActivePiercings;

	UPROPERTY(BlueprintReadOnly, Category = "Skin Overlay")
	TMap<FName, FActiveSkinOverlayState> SavedActiveDiseases;

	// --- 拘束具（足枷等）による移動制限の一元設定 ---
	// DT_equipments側の各行はON/OFFと種類の選択のみを持ち、実際の速度・ABPはここで一括管理する。

	/** 「早歩き」プリセット選択時の移動速度上限 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrained Movement")
	float RestrainedFastWalkSpeed = 300.0f;

	/** 「遅い歩き」プリセット選択時の移動速度上限 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrained Movement")
	float RestrainedSlowWalkSpeed = 150.0f;

	/** 拘束具装備中に差し替えるAnimBlueprint（ブレンドスペースを差し替えたABPを複製して設定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrained Movement")
	TSoftClassPtr<class UAnimInstance> RestrainedAnimBlueprintClass;

	/** プリセットに対応する移動速度上限を取得する */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Restrained Movement")
	float GetRestrainedSpeed(ERestrainedSpeedPreset Preset) const
	{
		return Preset == ERestrainedSpeedPreset::SlowWalk ? RestrainedSlowWalkSpeed : RestrainedFastWalkSpeed;
	}

	// --- セーブ/ロード ---

	/** ロード予約データ。LoadSavedGameでセットされ、レベル移動後にキャラクターのBeginPlayから読み込まれてクリアされる */
	UPROPERTY()
	class UMyProject1SaveGame* PendingLoadSaveGame = nullptr;

	/** 手動セーブスロットの数（"SaveSlot1"〜"SaveSlot8"）。オートセーブスロットはこれとは別に1つ持つ。 */
	static constexpr int32 NumManualSaveSlots = 8;

	/** オートセーブ専用スロットの内部名 */
	static const FString AutoSaveSlotName;

	/** 手動スロット番号（1〜NumManualSaveSlots）から内部スロット名（"SaveSlot3"等）を作る */
	UFUNCTION(BlueprintPure, Category = "Save")
	static FString GetManualSaveSlotName(int32 SlotIndex);

	/** レベル名（PlayerLevelName）→ セーブ画面に出す場所の表示名の対応表。未登録のレベルはレベル名をそのまま表示する。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	TMap<FName, FText> LevelDisplayNameMap;

	/** 現在のプレイヤー状態をまとめてスロットに保存する */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveCurrentGame(const FString& SlotName = TEXT("SaveSlot1"));

	/** オートセーブスロットへ保存する（呼び出しタイミングは呼ぶ側で決める。SaveCurrentGame(AutoSaveSlotName)の薄いラッパー） */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool AutoSaveGame();

	/** スロットからセーブデータを読み込み、既存のワープ着地機構に相乗りしてプレイヤーへ反映する */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadSavedGame(const FString& SlotName = TEXT("SaveSlot1"));

	/** 指定したスロットにセーブデータが存在するか（UIの「つづきから」表示用） */
	UFUNCTION(BlueprintPure, Category = "Save")
	bool DoesSaveGameExist(const FString& SlotName = TEXT("SaveSlot1")) const;

	/** セーブ画面の一覧用：手動スロット1〜5＋オートセーブの計(NumManualSaveSlots+1)件の見出し情報を返す。
	 *  先頭がオートセーブ、続いて手動スロット1〜5の順で並ぶ。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FSaveSlotDisplayInfo> GetAllSaveSlotInfos() const;

	/** 指定した1スロット分の見出し情報を返す（セーブ直後にその行だけ更新したい時に使う）。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	FSaveSlotDisplayInfo GetSaveSlotInfo(const FString& SlotName) const;

	/** レベル移動後、新しく生成されたキャラクターのBeginPlayから呼ばれ、PendingLoadSaveGameの中身を実際に適用する。
	 *  スナップショット（セーブロード or 別マップワープ）を実際に消費して復元したときだけ true を返す。
	 *  false のときは「完全新規開始」なので、呼び出し側がデフォルト装備などの初期化を行ってよい。 */
	bool ApplyPendingCharacterLoad(class AMyProject1Character* Character);

	// --- ログウィンドウの履歴 ---
	// レベル移動（OpenLevel）でWBP_LogWindowが再生成されても消えないよう、
	// GameInstance側に「箱の中身」のコピーを保持しておく（SavedActiveTattoos等と同じ仕組み）。

	/** 保持する最大件数（超えたら古いものから削除する） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	int32 MaxLogHistoryEntries = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Log")
	TArray<FLogHistoryEntry> LogHistory;

	/** ログウィンドウに新しい1件が表示された時、BP側（WBP_LogWindowのAddLogEntry）から呼んで履歴に積む */
	UFUNCTION(BlueprintCallable, Category = "Log")
	void AddLogHistoryEntry(const FString& Message, ELogMessageType InLogType);

	// --- イベント分岐システム ---
	// UEventDistributorComponentの抽選で決まったEventID（DT_EventDefinitionsの行名）を受け取り、
	// 対応する施設（WarpID）へワープさせてイベントを開始する。WarpDataTableと同じ「単一の参照をここに設定する」方式。

	/** イベント定義（DT_EventDefinitions）のデータテーブル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	UDataTable* EventDefinitionDataTable;

	/** 現在イベント進行中か（各トリガーが二重にイベントを発生させないためのガードにも使う） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bHasActiveEvent = false;

	/** 進行中のイベントID（DT_EventDefinitionsの行名） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName ActiveEventID;

	/** EventDistributorComponentの抽選で決まったEventIDを渡し、対応する施設（WarpID）へワープしてイベントを開始する。
	 *  ClearCondition=TimeElapsed/Bothの場合はTimeLimitSeconds後に自動でResolveActiveEvent(true)を呼ぶ。
	 *  ClearCondition=AnimationSequenceの場合は暗転明け後にAnimEventIDのステップ再生を開始し、完走で自動成立する。
	 *  EventContextActorはTriggerEventPoolを呼び出したOwnerActor（NPC/敵など）。FAnimEventStep::PlayTarget=NPC時の再生対象になる */
	UFUNCTION(BlueprintCallable, Category = "Event")
	void StartEvent(FName EventID, class ACharacter* PlayerCharacter, class AActor* EventContextActor = nullptr);

	/** 施設側のクリア判定（インタラクト等）、または制限時間切れから呼ばれる。bSuccess=trueなら成立、falseなら不成立として
	 *  対応するアクション群（SuccessActions/FailureActions）を実行し、ReturnWarpIDへ戻す */
	UFUNCTION(BlueprintCallable, Category = "Event")
	void ResolveActiveEvent(bool bSuccess);

	// --- アニメーションシーケンス再生（イベント分岐システムとは独立） ---
	// DT_AnimSequences（行名=個別ID、Tagでカテゴリ分類）とDT_AnimEvents（行名=AnimEventID、Steps列）の2テーブルで管理する。
	// PlayAnimSequenceEventはbHasActiveEvent等のイベント状態を一切見ないため、会話アクションやQuestItemPointのインタラクトなど
	// イベントを経由しない箇所からも単体で呼び出せる。イベント分岐システム側（ClearCondition=AnimationSequence）は
	// このPlayAnimSequenceEventを呼ぶ薄いラッパー（BeginAnimEventSequenceIfNeeded）になっている。

	/** アニメーション本体（DT_AnimSequences）のデータテーブル。行名は個別ID、Tagでカテゴリ分類する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimEvent")
	UDataTable* AnimSequenceDataTable;

	/** アニメーションイベント（DT_AnimEvents）のデータテーブル。行名がAnimEventIDになる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimEvent")
	UDataTable* AnimEventDataTable;

	/** PlayAnimSequenceEventの完了通知（全ステップ再生完了、または開始できず即失敗のどちらでもBroadcastされる） */
	UPROPERTY(BlueprintAssignable, Category = "AnimEvent")
	FOnAnimSequenceEventFinished OnAnimSequenceEventFinished;

	/** AnimEventID（DT_AnimEventsの行名）のStepsを先頭から順に再生する。イベント分岐システムを経由せず単体で呼び出せる。
	 *  PrimaryCharacterはFAnimEventStep::PlayTarget=Player時の再生対象。SecondaryContextActorはPlayTarget=NPC時の再生対象
	 *  （例：会話中のNPCや、EventDistributorComponentが付いているOwnerActor）。完了時にOnAnimSequenceEventFinishedをBroadcastする。
	 *  bFadeInBeforeStart=trueなら、Step0の再生前にもステップ切り替えと同じ暗転を挟む。
	 *  イベント分岐システム経由（BeginAnimEventSequenceIfNeeded）は既にワープ暗転明け直後なのでfalseのまま呼ぶ想定。
	 *  会話・QuestItemPointなどワープを経由しない直接呼び出しはtrueを渡すことで開始時にも暗転させられる */
	UFUNCTION(BlueprintCallable, Category = "AnimEvent")
	void PlayAnimSequenceEvent(FName AnimEventID, class ACharacter* PrimaryCharacter, class AActor* SecondaryContextActor = nullptr, bool bFadeInBeforeStart = false);

private:
	// 進行中イベントの制限時間タイマー、および対象プレイヤーの記憶（StartEvent/ResolveActiveEvent用）
	FTimerHandle ActiveEventTimeLimitTimerHandle;
	TWeakObjectPtr<class ACharacter> ActiveEventPlayer;

	// StartEventに渡されたEventContextActor（TriggerEventPoolを呼んだOwnerActor）の記憶。FAnimEventStep::PlayTarget=NPC用
	TWeakObjectPtr<class AActor> ActiveEventContextActor;

	/** ActiveEventTimeLimitTimerHandleから呼ばれ、制限時間経過による成立（ResolveActiveEvent(true)）を行う */
	void HandleActiveEventTimeUp();

	// 進行中イベントがClearCondition=AnimationSequenceの場合に、既にPlayAnimSequenceEventを開始済みか
	// （BeginAnimEventSequenceIfNeededの二重起動ガード。アニメーション再生自体の進行状態ではない）
	bool bAnimEventSequenceStarted = false;

	/** HandleWarpFadeInCompleteから呼ばれる。進行中イベントのClearConditionがAnimationSequenceで、
	 *  まだ再生を開始していない場合のみPlayAnimSequenceEventを開始し、完了通知（OnAnimSequenceEventFinished）を
	 *  HandleAnimSequenceEventFinishedForActiveEventで受けてResolveActiveEvent(true)へつなぐ */
	void BeginAnimEventSequenceIfNeeded();

	/** OnAnimSequenceEventFinishedのハンドラ。bHasActiveEvent中にBeginAnimEventSequenceIfNeeded経由で開始された
	 *  再生が完了した時だけResolveActiveEvent(true)を呼ぶ（単体でのPlayAnimSequenceEvent呼び出し時は何もしない） */
	UFUNCTION()
	void HandleAnimSequenceEventFinishedForActiveEvent(bool bCompletedNormally);

	// --- アニメーションシーケンス再生（PlayAnimSequenceEvent）自体の進行状態。イベント分岐システムの状態とは独立 ---
	FName CurrentAnimEventID;
	TWeakObjectPtr<class ACharacter> AnimEventPrimaryCharacter;
	TWeakObjectPtr<class AActor> AnimEventSecondaryContextActor;
	int32 CurrentAnimEventStepIndex = INDEX_NONE;
	bool bAnimEventStepLooping = false;
	FTimerHandle AnimEventStepDurationTimerHandle;

	// 現在のステップで実際に再生中のモンタージュ（PlayAnimEventStepで抽選した1本）。
	// Montage_Stopで打ち切った古いステップのモンタージュから遅延して届くOnMontageEndedを、
	// HandleAnimEventStepMontageEndedが新しいステップの状態と誤って結びつけないための照合に使う
	TWeakObjectPtr<class UAnimMontage> CurrentAnimEventMontage;

	// FAnimEventPairing::ParticipantID → そのIDのためにスポーンした表示専用Character（AAnimEventActor）。
	// AnimEventの開始時は空。抽選で選ばれたFAnimSequenceEntry::ExtraPairingsにそのIDが初登場したStepで
	// スポーンし、全Step完了・イベント強制終了のどちらでもDestroyAnimEventExtraActorsで破棄してクリアする
	TMap<FName, TWeakObjectPtr<class AAnimEventActor>> AnimEventExtraActors;

	// 各Extra参加者のスポーン時点でのMesh相対Transform（AnimEventExtraActorsと同じKey）。
	// Primary/Secondaryと同じ「基準値＋FAnimSequenceEntryのOffset」方式に揃えるための基準値
	TMap<FName, FVector> AnimEventExtraBaseMeshLocations;
	TMap<FName, FRotator> AnimEventExtraBaseMeshRotations;

	// 現在のステップで各Extra参加者が実際に再生中のモンタージュ（AnimEventExtraActorsと同じKey）。
	// CurrentAnimEventMontageのExtra参加者版
	TMap<FName, TWeakObjectPtr<class UAnimMontage>> CurrentAnimEventExtraMontages;

	// FAnimSequenceEntry::PropMeshでスポーンした小道具（椅子等、Player側のみ）。
	// 行の再生開始時にSpawnOrUpdateAnimSequencePropでスポーン/更新し、DestroyAnimEventExtraActorsで一緒に破棄する
	TWeakObjectPtr<class AStaticMeshActor> CurrentAnimSequencePropActor;

	// PlayAnimSequenceEventがFAnimEventDefinition::EventBGMでBGMをオーバーライドしたか
	// （trueの場合のみ、全ステップ完了時にAMyProject1Character::MusicComp->ExitRoomMusic()で元のBGMへ戻す）
	bool bAnimEventOverrodeMusic = false;

	// PlayAnimSequenceEvent開始時点でのPrimary/SecondaryのMesh相対Transform（FAnimSequenceEntry::MeshLocationOffset/
	// MeshRotationOffset適用前の基準値）。各ステップ開始時はこの基準値+Offsetを都度設定し、全ステップ完了時にこの値へ戻す
	FVector AnimEventPrimaryBaseMeshLocation = FVector::ZeroVector;
	FRotator AnimEventPrimaryBaseMeshRotation = FRotator::ZeroRotator;
	FVector AnimEventSecondaryBaseMeshLocation = FVector::ZeroVector;
	FRotator AnimEventSecondaryBaseMeshRotation = FRotator::ZeroRotator;

	// ステップ切り替え時の暗転演出（試験実装）。ワープと同じOnWarpFadeOutRequestedをUIへBroadcastし、
	// WarpFadeOutDuration秒後（画面が真っ暗になったタイミング）でPendingAnimEventNextStepIndexへ切り替える
	int32 PendingAnimEventNextStepIndex = INDEX_NONE;
	FTimerHandle AnimEventStepTransitionTimerHandle;

	// 非ループステップ用：Montage自体の残り再生時間がWarpFadeOutDuration秒を切ったタイミングで暗転を
	// 開始するためのタイマー。Montage_SetBlendingOutDelegate（Montage自身の短いBlendOutTime基準）だと、
	// 画面が完全に暗くなるまでの時間（WarpFadeOutDuration）の方が長い場合に暗転完了前にIdleが透けて見えるため、
	// 「暗転にちょうどWarpFadeOutDuration秒かかる」ことを見越して、その分だけ早く暗転を開始する
	FTimerHandle AnimEventStepFadeLeadTimerHandle;

	/** AnimEventStepFadeLeadTimerHandleから呼ばれ、Montageの残りがWarpFadeOutDuration秒を切った
	 *  タイミングで次のステップへの暗転（TransitionToAnimEventStep）を開始する。ループ中、または
	 *  既に他経路（BlendingOut/End）で暗転済みの場合は何もしない */
	UFUNCTION()
	void HandleAnimEventStepFadeLeadTimeUp();

	/** 現在のステップ再生を終えて次のステップへ進む際に、直接PlayAnimEventStepを呼ぶ代わりに使う。
	 *  OnWarpFadeOutRequestedをBroadcastしてから、WarpFadeOutDuration秒後にPlayAnimEventStep(NextStepIndex)を呼ぶ */
	void TransitionToAnimEventStep(int32 NextStepIndex);

	/** AnimEventStepTransitionTimerHandleから呼ばれ、暗転済みのタイミングで次のステップの再生を開始する */
	void HandleAnimEventStepTransitionFadeComplete();

	/** AnimEventDataTableのStepsをStepIndexから再生する。範囲外（=全ステップ再生完了）ならOnAnimSequenceEventFinished(true)をBroadcastする */
	void PlayAnimEventStep(int32 StepIndex);

	/** bLoop=trueのステップでAnimEventStepDurationTimerHandleから呼ばれる。タイマー自体はStep.Durationから
	 *  WarpFadeOutDuration分を差し引いた時点で発火するようセットされており、ここではまだモンタージュを止めず
	 *  次のステップへの暗転（TransitionToAnimEventStep）を開始するだけにする。実際にモンタージュを止めるのは
	 *  暗転が完全に終わった後（HandleAnimEventStepTransitionFadeComplete）。これによりトータルの再生時間は
	 *  Step.Durationのまま、打ち切り自体は暗転で隠された状態で行われる */
	void HandleAnimEventStepDurationTimeUp();

	/** Montage_SetEndDelegateで張った動的デリゲート。bLoop=trueの場合はループの継ぎ目の再生し直しを
	 *  HandleAnimEventStepMontageBlendingOut側に任せるため何もせず、bLoop=falseの場合のみ次のステップへ進む */
	UFUNCTION()
	void HandleAnimEventStepMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Montage_SetBlendingOutDelegateで張った動的デリゲート。bLoop=true中、Montageが自然終了して
	 *  ブレンドアウトを開始した瞬間（＝ウェイトがまだ高いうち）に同じモンタージュを再生し直すことで、
	 *  HandleAnimEventStepMontageEnded（ブレンドアウト完了＝Idleに一度戻ってから発火）を使うより
	 *  ループの継ぎ目でIdleへ一瞬戻って見える現象を防ぐ。bInterrupted=true（Duration経過等による
	 *  明示的なMontage_Stop）の場合は再生し直さない */
	UFUNCTION()
	void HandleAnimEventStepMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	/** TargetのAnimInstanceでMontageを再生し、End/BlendingOutの両デリゲートを結び直す共通処理。
	 *  PlayAnimEventStepでの初回再生と、HandleAnimEventStepMontageBlendingOutでのループ再生し直しの両方から呼ぶ */
	void PlayAnimEventStepMontage(class UAnimInstance* AnimInst, UAnimMontage* Montage);

	/** Pairing.ParticipantIDのAAnimEventActorが未スポーンならAnimEventPrimaryCharacter基準の位置・回転でスポーンする。
	 *  スポーン済みならそれを返し、AnimEventPrimaryCharacter不在等で失敗した場合はnullptrを返す */
	class AAnimEventActor* GetOrSpawnAnimEventExtraActor(const FAnimEventPairing& Pairing);

	/** 抽選で選ばれたFAnimSequenceEntry::ExtraPairingsの1件分を再生する
	 *  （未登場ならスポーン→Tagから抽選→オフセット適用→モンタージュ再生→セリフ表示）。
	 *  再生できればそのモンタージュの長さを、Tag未設定・アセット不備等で再生できなければ負値を返す。
	 *  bLoopUntilStopped=true（PlayAnimSequenceRowDirect専用）の場合、Stepシステムのデリゲートの代わりに
	 *  HandleAnimSequenceRowDirectExtraMontageBlendingOutを使い、明示的に止める（Destroy）までループし続ける */
	float PlayAnimEventPairing(const FAnimEventPairing& Pairing, bool bLoopUntilStopped = false);

	/** AnimEventExtraActorsに残っている全Extra参加者と、CurrentAnimSequencePropActorを破棄してクリアする。
	 *  全Step再生完了時、およびResolveActiveEventによるイベント強制終了時（アニメ再生中の時間切れ等）の両方から呼ぶ */
	void DestroyAnimEventExtraActors();

	/** Entry.PropMeshが設定されていれば、PrimaryCharacter基準のPropRelativeLocation/Rotationへ
	 *  CurrentAnimSequencePropActorをスポーン（未スポーンの場合）またはメッシュ差し替え・位置更新する。
	 *  PropMesh未設定なら何もしない。PlayAnimEventStep・PlayAnimSequenceRowDirectの両方（Player再生時のみ）から呼ぶ */
	void SpawnOrUpdateAnimSequenceProp(const FAnimSequenceEntry& Entry);

	/** Montageがメイン参加者（CurrentAnimEventMontage）またはExtra参加者（CurrentAnimEventExtraMontages）のいずれかで
	 *  現在再生中として記録されているかを判定する。End/BlendingOutデリゲートが古いステップの残骸を誤って処理しないためのガード */
	bool IsTrackedAnimEventMontage(UAnimMontage* Montage) const;

	/** IsTrackedAnimEventMontageで一致したMontageについて、それを再生しているAnimInstanceを解決する
	 *  （ループ継ぎ目の再生し直しで、どのキャラクターのAnimInstanceにMontage_Playし直すか決めるために使う） */
	class UAnimInstance* ResolveAnimInstanceForTrackedMontage(UAnimMontage* Montage) const;

	// --- DT_AnimSequencesの1行を直接再生するテスト用機能（位置調整確認用。DT_AnimEvents/Steps/Tag抽選/
	// BGM切替/暗転演出は一切経由しない）。PlayAnimSequenceEvent系の状態（AnimEventPrimaryCharacter等）を
	// 一部共有するため、DT_AnimEventsのStepが進行中（CurrentAnimEventStepIndex != INDEX_NONE）の間は使えない ---
public:
	/** DT_AnimSequencesの指定行（RowName）のMontageを、PlayTargetで指定したPrimary(Player)/Secondary(NPC)側で
	 *  再生する。位置調整中はポーズを保てるよう、明示的に止める（StopAnimSequenceRowDirect）までループし続ける。
	 *  行のExtraPairingsも通常のイベント再生と同じ処理で同時にスポーン・再生するため（こちらは1回のみ）、
	 *  Player/NPC/追加参加者のMeshLocationOffset/MeshRotationOffsetによる位置関係をまとめて確認できる。
	 *  再生中に呼び直すと、前回のテスト再生（Extra参加者・位置オフセット）を片付けてから再生し直す */
	UFUNCTION(BlueprintCallable, Category = "AnimEvent")
	void PlayAnimSequenceRowDirect(FName RowName, class ACharacter* PrimaryCharacter, class AActor* SecondaryContextActor, EStatTargetActor PlayTarget);

	/** PlayAnimSequenceRowDirectでの再生中に、メイン対象のMeshLocationOffset/MeshRotationOffsetを
	 *  その場で加算調整する（現在値に累積）。BP側でキー入力に割り当てて使うことを想定。
	 *  再生中でなければ何もしない。調整後の値を画面にデバッグ表示する */
	UFUNCTION(BlueprintCallable, Category = "AnimEvent")
	void NudgeAnimSequenceRowDirectOffset(FVector LocationDelta, FRotator RotationDelta);

	/** DT_AnimSequencesの全行を、UI表示用の軽量データ一覧として取得する
	 *  （デバッグメニュー等がBP側で一覧を組み立てる際に使う。GetAllWarpDestinationsと同じ用途） */
	UFUNCTION(BlueprintCallable, Category = "AnimEvent")
	TArray<FAnimSequenceRowInfo> GetAllAnimSequenceRows() const;

private:
	/** PlayAnimSequenceRowDirectで動かしたPrimary/Secondaryのメッシュ位置を基準値へ戻し、ループ再生中の
	 *  モンタージュを止め、スポーンしたExtra参加者を破棄する。次のPlayAnimSequenceRowDirect呼び出し時の
	 *  後片付けと、PlayAnimSequenceEventが割り込む場合の両方から呼ぶ */
	void StopAnimSequenceRowDirect();

	/** PlayAnimSequenceRowDirectで再生したメイン対象のモンタージュのBlendingOutデリゲート。
	 *  位置調整中はポーズを保つため、ブレンドアウトが完了しきる前（bInterrupted=false）に
	 *  同じモンタージュを再生し直してループを継続する。StopAnimSequenceRowDirectによる明示的な
	 *  Montage_Stop（bInterrupted=true）の場合はループし直さない */
	UFUNCTION()
	void HandleAnimSequenceRowDirectMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	/** PlayAnimEventPairing(bLoopUntilStopped=true)で再生したExtra参加者のモンタージュのBlendingOutデリゲート。
	 *  HandleAnimSequenceRowDirectMontageBlendingOutのExtra参加者版で、同じ理屈でループし続ける。
	 *  対象のExtra参加者はStopAnimSequenceRowDirect（DestroyAnimEventExtraActors）でActorごと破棄されるため、
	 *  破棄後はResolveAnimInstanceForTrackedMontageがnullptrを返して自然にループが止まる */
	UFUNCTION()
	void HandleAnimSequenceRowDirectExtraMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	// PlayAnimSequenceRowDirectで現在ループ再生中のメイン対象モンタージュ。
	// 有効な間＝テスト再生中の判定にも使う（PlayAnimSequenceEventとの状態競合ガード、Nudgeの有効判定）
	TWeakObjectPtr<class UAnimMontage> CurrentAnimSequenceRowDirectMontage;

	// CurrentAnimSequenceRowDirectMontageをPrimary(Player)/Secondary(NPC)のどちらで再生しているか
	// （Nudge・ループ再生し直しの際にどちらのメッシュ・基準値を使うか判定するため）
	EStatTargetActor CurrentAnimSequenceRowDirectPlayTarget = EStatTargetActor::Player;

	// NudgeAnimSequenceRowDirectOffsetで累積調整中の値（初期値はFAnimSequenceEntry::MeshLocationOffset/
	// MeshRotationOffset）。基準値（AnimEventPrimary/SecondaryBaseMeshLocation/Rotation）に加算して適用する
	FVector CurrentAnimSequenceRowDirectLocationOffset = FVector::ZeroVector;
	FRotator CurrentAnimSequenceRowDirectRotationOffset = FRotator::ZeroRotator;


	// ★追加：暗転が終わるまで待機している「ワープID」と「プレイヤー」の記憶
	FName ReservedWarpID;
	TWeakObjectPtr<class ACharacter> ReservedPlayer;

	// ★追加：暗転演出（暗転→ExecuteWarpProcess→明転→入力復帰）を自走させるためのタイマーと記憶領域
	FTimerHandle WarpFadeOutTimerHandle;
	FTimerHandle WarpFadeInTimerHandle;

	/** 暗転中に入力を止めた対象（明転後にEnableInputで戻す用）。
	 *  レベル跨ぎワープでOpenLevelにより対象が破棄された場合はGet()がnullを返すため、
	 *  HandleWarpFadeInComplete側で安全にスキップされる（新しいPawnはbBlockInput=falseがデフォルトなので対応不要） */
	TWeakObjectPtr<class ACharacter> InputDisabledCharacter;

	/** 暗転開始の共通処理：入力停止＋暗転タイマー予約＋UIへ合図。各Request系関数から呼ぶ */
	void BeginWarpFade(class ACharacter* TargetCharacter);

	/** 暗転が終わった（＝画面が真っ暗になった）タイミングでタイマーから呼ばれ、実際の移動処理を実行する */
	void HandleWarpFadeOutComplete();

	/** 明転が終わったタイミングでタイマーから呼ばれ、入力を戻す */
	void HandleWarpFadeInComplete();

	// 暗転が終わるまで待機している「付与予定のフラグ」と「付与対象」の記憶（RequestFadeThenGrantFlag用）
	FName ReservedFlagToGrant;
	TWeakObjectPtr<class AMyProject1Character> ReservedFlagGrantTarget;

	// 暗転が終わるまで待機している「消去予定のフラグ」と「消去対象」の記憶（RequestFadeThenRemoveFlag用）
	FName ReservedFlagToRemove;
	TWeakObjectPtr<class AMyProject1Character> ReservedFlagRemoveTarget;

	// 暗転が終わるまで待機している「進める分数」と「対象」の記憶（RequestFadeThenAdvanceTime用）
	int32 ReservedTimeSkipMinutes = 0;
	TWeakObjectPtr<class ACharacter> ReservedTimeSkipCharacter;
	bool ReservedTimeSkipIsSleep = false;

	// 暗転が終わるまで待機している「ワープ元のWallWarpLink」と「対象キャラクター」の記憶（RequestFadeThenWallWarp用）
	TWeakObjectPtr<class AWallWarpLink> ReservedWallWarpLink;
	TWeakObjectPtr<class ACharacter> ReservedWallWarpCharacter;

	/** 現在のプレイヤー状態を新しいUMyProject1SaveGameへ複製する（ディスクへは書き込まない一時オブジェクト）。
	 *  ディスクへのセーブ（SaveCurrentGame）と、別マップへのワープでキャラクターが再生成される際の
	 *  状態引き継ぎ（ExecuteWarpProcess）の両方から使う共通処理。 */
	class UMyProject1SaveGame* CapturePlayerStateSnapshot(class AMyProject1Character* Character);

protected:
	// 時間を計算して進めるタイマーの本体
	void UpdateInGameTime();
	FTimerHandle TimeUpdateTimerHandle;

	// 日付を1日進める内部処理
	void AdvanceDay();

	// その月が何日あるか（月末）を判定する計算関数
	int32 GetDaysInMonth(int32 Year, int32 Month);

};