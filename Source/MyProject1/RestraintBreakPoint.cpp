#include "RestraintBreakPoint.h"
#include "Components/StaticMeshComponent.h"
#include "MyProject1Character.h"
#include "MyProject1Types.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

ARestraintBreakPoint::ARestraintBreakPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// ASleepPointと同じ：Tag "NPC" を付けるだけで既存のInteractRange判定に乗る
	Tags.Add(FName("NPC"));

	// 複数ロック時の破壊優先順位の既定：足輪 → 腕輪 → 首
	BreakPriorityOrder = { EEquipmentSlot::Ankle, EEquipmentSlot::Wrist, EEquipmentSlot::Neck };
}

void ARestraintBreakPoint::TryInteract(AMyProject1Character* Interactor)
{
	if (!Interactor) return;

	// 成功・失敗を問わず、インタラクト時点でターゲットカーソルを消す（ASleepPointと同じ作法）
	Interactor->CancelTarget();

	const TArray<EEquipmentSlot> LockedSlots = Interactor->GetLockedEquipmentSlots();

	if (LockedSlots.Num() == 0)
	{
		Interactor->OnReceiveLogMessage(TEXT("ここで壊せそうな拘束具はない。"), ELogMessageType::System);
		return;
	}

	if (LockedSlots.Num() == 1)
	{
		TryBreakSlot(Interactor, LockedSlots[0]);
		return;
	}

	// 複数ある：BP側で選択UI（宝箱UI流用）を出してもらい、選んだらTryBreakSlotを呼び戻す。
	// BP未実装なら _Implementation の既定動作（先頭を自動選択）が走る。
	OnNeedSlotSelection(Interactor, LockedSlots);
}

void ARestraintBreakPoint::OnNeedSlotSelection_Implementation(AMyProject1Character* Interactor, const TArray<EEquipmentSlot>& LockedSlots)
{
	// 既定：選択UIが無いので、BreakPriorityOrder（足輪→腕輪→首）に従って対象を1つ選ぶ
	if (!Interactor || LockedSlots.Num() == 0) return;

	for (EEquipmentSlot PrioritySlot : BreakPriorityOrder)
	{
		if (LockedSlots.Contains(PrioritySlot))
		{
			TryBreakSlot(Interactor, PrioritySlot);
			return;
		}
	}

	// 優先リストに該当が無ければ先頭
	TryBreakSlot(Interactor, LockedSlots[0]);
}

void ARestraintBreakPoint::TryBreakSlot(AMyProject1Character* Interactor, EEquipmentSlot TargetSlot)
{
	if (!Interactor) return;

	// 選択UIを挟む間に状況が変わっている可能性があるので、まだロック装備であることを確認
	if (!Interactor->IsSlotLocked(TargetSlot))
	{
		Interactor->OnReceiveLogMessage(TEXT("対象の拘束具はもう無い。"), ELogMessageType::System);
		return;
	}

	const bool bSuccess = (FMath::FRand() < BreakSuccessChance);

	if (bSuccess)
	{
		// 破壊解除：インベントリには戻さず消滅させる
		Interactor->ForceRemoveLockedEquipment(TargetSlot, /*bReturnToInventory=*/false);
		Interactor->OnReceiveLogMessage(TEXT("拘束具を破壊して外した。"), ELogMessageType::System);
	}
	else
	{
		Interactor->OnReceiveLogMessage(TEXT("うまく壊せなかった…。"), ELogMessageType::System);
	}

	OnBreakAttempted(bSuccess);
}
