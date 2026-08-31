#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "MyProject1Types.h" // ★ワープ先の構造体を使うために追加
#include "MyProject1GameInstance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarpFadeOutRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnInGameTimeChanged, int32, Year, int32, Month, int32, Day, int32, Hour, int32, Minute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDayChangedSignature);

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

	/** 現在のプレイヤー状態をまとめてスロットに保存する */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveCurrentGame(const FString& SlotName = TEXT("SaveSlot1"));

	/** スロットからセーブデータを読み込み、既存のワープ着地機構に相乗りしてプレイヤーへ反映する */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadSavedGame(const FString& SlotName = TEXT("SaveSlot1"));

	/** 指定したスロットにセーブデータが存在するか（UIの「つづきから」表示用） */
	UFUNCTION(BlueprintPure, Category = "Save")
	bool DoesSaveGameExist(const FString& SlotName = TEXT("SaveSlot1")) const;

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

private:
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