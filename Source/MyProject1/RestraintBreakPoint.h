#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Types.h"
#include "RestraintBreakPoint.generated.h"

/**
 * 拘束具・呪物を「破壊して」外すためのインタラクトポイント（作業台・岩・鍛冶炉など）。
 * ASleepPointと同じ「Tagsに"NPC"を付けてInteractRange判定に乗せる」方式。
 *
 * ・成功率はこのアクタが持つ（頑丈な作業台ほど高い、など個体ごとに設定）。
 * ・成功したらForceRemoveLockedEquipment(Slot, bReturnToInventory=false)で装備を消滅させる（破壊なのでインベントリに戻さない）。
 * ・ロック装備が複数あるときはOnNeedSlotSelectionをBPへ投げ、BP側の選択UI（宝箱UI流用）からTryBreakSlotを呼び戻す。
 */
UCLASS()
class MYPROJECT1_API ARestraintBreakPoint : public AActor
{
	GENERATED_BODY()

public:
	ARestraintBreakPoint();

	// --- 見た目 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* Mesh;

	/** 破壊解除の成功率（0.0〜1.0）。このポイント固有の値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RestraintBreak", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BreakSuccessChance = 0.3f;

	/**
	 * ロック装備が複数あるとき、選択UIが無い場合にどのスロットから壊すかの優先順位。
	 * 既定は 足輪(Ankle) → 腕輪(Wrist) → 首(Neck)。リストに無いスロットしか無ければ先頭を使う。
	 * コンストラクタで既定値をセット。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RestraintBreak")
	TArray<EEquipmentSlot> BreakPriorityOrder;

	/** プレイヤーがインタラクトした時に呼ぶ入口。ロック装備0個＝何もしない／1個＝即試行／複数＝OnNeedSlotSelection */
	UFUNCTION(BlueprintCallable, Category = "RestraintBreak")
	void TryInteract(class AMyProject1Character* Interactor);

	/** 対象スロットを確定して破壊を1回試行する。複数ロック時はBPの選択UIからこれを呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "RestraintBreak")
	void TryBreakSlot(class AMyProject1Character* Interactor, EEquipmentSlot TargetSlot);

	/**
	 * ロック装備が複数あるので対象を選んでほしい、という合図。
	 * BP未実装のときのC++既定動作＝先頭のロックスロットを自動選択してTryBreakSlotを呼ぶ。
	 * BPでオーバーライドすれば選択UI（宝箱UI流用）を出し、選んだスロットで TryBreakSlot(Interactor, Slot) を呼ぶ。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "RestraintBreak")
	void OnNeedSlotSelection(class AMyProject1Character* Interactor, const TArray<EEquipmentSlot>& LockedSlots);
	void OnNeedSlotSelection_Implementation(class AMyProject1Character* Interactor, const TArray<EEquipmentSlot>& LockedSlots);

	/** 破壊試行の結果通知（BP側でエフェクト・SEを鳴らす用） */
	UFUNCTION(BlueprintImplementableEvent, Category = "RestraintBreak")
	void OnBreakAttempted(bool bSuccess);
};
