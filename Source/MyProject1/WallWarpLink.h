#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Types.h" // EWallWarpTriggerType
#include "WallWarpLink.generated.h"

/**
 * 壁一枚を挟んだ反対側などへ、同一レベル内で軽量に移動するためのワープリンク。
 * DT_WarpDestinations/GameInstance/ロード画面を一切介さず、TargetLinkが指す
 * 相方のAWallWarpLinkへ直接テレポートするだけの単純な仕組み。AWarpPortal（レベル跨ぎ用）とは別系統。
 * 2つを1組としてレベルに配置し、互いのTargetLinkに相手を指定して使う。
 */
UCLASS()
class MYPROJECT1_API AWallWarpLink : public AActor
{
	GENERATED_BODY()

public:
	AWallWarpLink();

protected:
	virtual void BeginPlay() override;

public:
	// --- コンポーネント ---
	/** 当たり判定（触れたことを検知するボックス） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* CollisionBox;

	// --- リンク設定（エディタで自由に設定可能） ---
	/** 移動先となる、相方のワープリンク（このレベル内に配置した別のAWallWarpLinkを指定） */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Wall Warp Settings")
	AWallWarpLink* TargetLink = nullptr;

	/** ワープの起動方法（触れるか、調べるか） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Warp Settings")
	EWallWarpTriggerType TriggerType = EWallWarpTriggerType::TouchToWarp;

	/** 空欄でなければ、プレイヤーがこのフラグを持つまで非表示・当たり判定OFFにしておく
	    （NPCと会話してAddFlagされるまでワープの存在自体を隠したい時に使う） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Warp Settings")
	FName RequiredFlag;

	/** 移動後、このリンクの前方（矢印の向き）にどれだけずらして出すか。0だとTargetLinkの直上に出て、
	    往復ワープしてしまう恐れがあるため、壁の厚み分だけ相手の外側に出す想定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Warp Settings")
	float ExitOffsetDistance = 100.0f;

	/** 移動直後、同じキャラクターが連続でこのリンクに再突入してもワープしない猶予時間（秒）。
	    ExitOffsetDistanceだけでは往復トリガーを防ぎきれない配置向けの保険 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Warp Settings")
	float ReTriggerCooldown = 0.5f;

	// --- 実行関数 ---
	/** キャラクターがボックスに触れた時に呼ばれる */
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** キャラクターがボックスから離れた時に呼ばれる */
	UFUNCTION()
	void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** プレイヤーがEキーなどで「調べた」時に呼ばれる（BP側から呼ぶ） */
	UFUNCTION(BlueprintCallable, Category = "Wall Warp")
	void InteractWithLink(class ACharacter* Interactor);

	/** 現在ボックスに触れているキャラクターがいるか（=InteractToWarpのターゲット対象にしてよいか）。
	    ターゲッティングのInteractRange（固定半径）ではなく、実際にこの場所にいるかで判定させたい時に使う */
	UFUNCTION(BlueprintPure, Category = "Wall Warp")
	bool IsReadyToInteract() const { return bIsPlayerOverlapping; }

	/** 暗転が終わった瞬間にGameInstance（RequestFadeThenWallWarp経由）から呼ばれ、実際にキャラクターを
	    TargetLinkへ移動させる。往復トリガー防止のクールダウンもここでセットする */
	void ExecuteWarpNow(class ACharacter* TargetCharacter);

private:
	/** ワープ実行を要求する（GameInstance経由で暗転を挟み、実際の移動はExecuteWarpNowで行う） */
	void ExecuteWarp(class ACharacter* TargetCharacter);

	/** RequiredFlagの所持状況に応じて、表示/当たり判定のON・OFFを切り替える */
	void UpdateFlagVisibility(const class AMyProject1Character* PlayerChar);

	/** プレイヤーがフラグを獲得した時に呼ばれる（OnFlagAddedの購読先）。RequiredFlag一致時のみ表示を更新する */
	UFUNCTION()
	void OnPlayerFlagAdded(FName FlagName);

	/** 直近でこのリンク経由でワープしてきたキャラクターと、そのクールダウン終了時刻 */
	TMap<TWeakObjectPtr<AActor>, float> RecentlyArrivedActors;

	/** 現在ボックスにキャラクターが重なっているか */
	bool bIsPlayerOverlapping = false;
};
