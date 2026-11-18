#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Types.h" // EWarpPortalTypeなどを使うため
#include "WarpPortal.generated.h"

UCLASS()
class MYPROJECT1_API AWarpPortal : public AActor
{
	GENERATED_BODY()

public:
	AWarpPortal();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// --- コンポーネント ---
	/** 当たり判定（触れたことを検知するボックス） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* CollisionBox;

	// --- ワープ設定（エディタで自由に設定可能） ---
	/** 飛ぶ先のWarpID（DT_WarpDestinations に登録した行名） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	FName TargetWarpID;

	/** ワープの起動方法（触れるか、調べるか） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	EWarpPortalType PortalType = EWarpPortalType::TouchToWarp;

	// 確認時に呼び出す会話データ（DT_Dialogs）の行名
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	FName ConfirmDialogID;

	/** 空欄でなければ、プレイヤーがこのフラグを持つまで非表示・当たり判定OFFにしておく
	    （宿屋の主人との会話でAddFlagされるまでドアの存在自体を隠したい時に使う） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	FName RequiredFlag;

	/** ONにすると、RequiredFlagが立ってから AutoClearFlagGameHours（ゲーム内時間）経過した時点で
	    プレイヤーからRequiredFlagを自動で外す。フラグが外れるとこのポータルは再び非表示・当たり判定OFF（入れなくなる）。
	    出口側の別ポータル（RequiredFlag空欄）は影響を受けないので、「入れないが出られる」宿屋などに使える */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings")
	bool bAutoClearFlag = false;

	/** bAutoClearFlagがONの時、フラグが立ってから自動で外れるまでのゲーム内時間（時間単位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Settings", meta = (EditCondition = "bAutoClearFlag", ClampMin = "0.0"))
	float AutoClearFlagGameHours = 15.0f;

	// --- 処理関数 ---
	/** プレイヤーが触れた時に呼ばれる関数 */
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** プレイヤーがEキーなどで「調べた」時に呼ばれる関数（BPから呼ぶ） */
	UFUNCTION(BlueprintCallable, Category = "Warp")
	void InteractWithPortal(class ACharacter* Interactor);

private:
	/** 実際のワープ処理（GameInstanceを呼ぶ） */
	void ExecuteWarp(class ACharacter* TargetCharacter);

	/** RequiredFlag運用時の初期化。プレイヤー生成と、別レベルからのステータス復元(ApplyPendingCharacterLoad)を
	    待ってからRequiredFlagを評価する。復元はAddFlagを経由せずUnlockedFlagsを一括代入するためOnFlagAddedが
	    飛ばず、BeginPlay一度きりの判定ではレベル遷移後に取りこぼす。成立するまで短間隔でリトライする
	    （AQuestItemPoint::InitFlagVisibilityWhenReadyと同じ対策） */
	void InitFlagVisibilityWhenReady();

	/** InitFlagVisibilityWhenReadyのリトライ用タイマー・試行回数、および購読済みフラグ */
	FTimerHandle FlagInitTimerHandle;
	int32 FlagInitAttempts = 0;
	bool bFlagDelegatesBound = false;

	/** RequiredFlagの所持状況に応じて、表示/当たり判定のON・OFFを切り替える */
	void UpdateFlagVisibility(const class AMyProject1Character* PlayerChar);

	/** プレイヤーがフラグを獲得した時に呼ばれる（OnFlagAddedの購読先）。RequiredFlag一致時のみ表示を更新する */
	UFUNCTION()
	void OnPlayerFlagAdded(FName FlagName);

	/** プレイヤーがフラグを失った時に呼ばれる（OnFlagRemovedの購読先）。RequiredFlag一致時のみ再び非表示・当たり判定OFFに戻す */
	UFUNCTION()
	void OnPlayerFlagRemoved(FName FlagName);

	/** ゲーム内時間が進むたびに呼ばれる（OnInGameTimeChangedの購読先）。自動OFFの期限を過ぎていればフラグを外す */
	UFUNCTION()
	void OnInGameTimeChangedHandler(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute);

	/** 自動OFFの期限を過ぎていればRequiredFlagをプレイヤーから外す */
	void CheckAutoClearDeadline();

	/** 自動OFFの期限を（未設定なら）今からAutoClearFlagGameHours後に設定する */
	void StartAutoClearDeadline();
};