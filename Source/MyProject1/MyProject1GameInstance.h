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

	// ゲーム開始時に呼ばれる関数（ここでタイマーを動かします）
	virtual void Init() override;

	// --- ワープ設定 ---

	UPROPERTY(BlueprintAssignable, Category = "Warp")
	FOnWarpFadeOutRequested OnWarpFadeOutRequested;

	/** ワープ先名簿（データテーブル）をセットする場所 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp")
	UDataTable* WarpDataTable;

	// --- ワープの記憶領域 ---

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

	/** レベル移動後、新しく生成されたキャラクターのBeginPlayから呼ばれ、PendingLoadSaveGameの中身を実際に適用する */
	void ApplyPendingCharacterLoad(class AMyProject1Character* Character);

private:
	// ★追加：暗転が終わるまで待機している「ワープID」と「プレイヤー」の記憶
	FName ReservedWarpID;
	TWeakObjectPtr<class ACharacter> ReservedPlayer;

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