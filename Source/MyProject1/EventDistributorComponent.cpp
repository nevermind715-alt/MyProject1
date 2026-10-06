#include "EventDistributorComponent.h"
#include "MyProject1Character.h"
#include "MyProject1GameInstance.h"
#include "RpgCharacterInterface.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

UEventDistributorComponent::UEventDistributorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FName UEventDistributorComponent::RollEventPool(AMyProject1Character* PlayerCharacter, FName EventPoolIDOverride) const
{
	const FName PoolIDToUse = EventPoolIDOverride.IsNone() ? EventPoolID : EventPoolIDOverride;
	if (!PlayerCharacter || !EventPoolDataTable || PoolIDToUse.IsNone()) return NAME_None;

	FEventPoolData* PoolData = EventPoolDataTable->FindRow<FEventPoolData>(PoolIDToUse, TEXT("RollEventPool"));
	if (!PoolData || PoolData->Entries.Num() == 0) return NAME_None;

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerCharacter);

	// RequiredFlagを満たす候補だけを対象に、Weightの相対比率で重み付き抽選する（合計が100である必要はない）
	TArray<const FEventPoolEntry*> ValidEntries;
	float TotalWeight = 0.0f;
	for (const FEventPoolEntry& Entry : PoolData->Entries)
	{
		if (Entry.Weight <= 0.0f) continue;
		if (!Entry.RequiredFlag.IsNone() && (!RpgInterface || !RpgInterface->HasFlag(Entry.RequiredFlag))) continue;

		ValidEntries.Add(&Entry);
		TotalWeight += Entry.Weight;
	}

	if (ValidEntries.Num() == 0 || TotalWeight <= 0.0f) return NAME_None;

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	FName ChosenEventID = NAME_None;
	bool bEntryChosen = false;
	for (const FEventPoolEntry* Entry : ValidEntries)
	{
		Roll -= Entry->Weight;
		if (Roll <= 0.0f)
		{
			ChosenEventID = Entry->EventID;
			bEntryChosen = true;
			break;
		}
	}

	// 浮動小数点誤差でどの候補にも当たらなかった場合の保険（末尾候補を採用する）。
	// EventIDが空欄（＝「何も起きない」用のダミー候補）の候補が正しく当選した場合はこの保険には入らない
	if (!bEntryChosen)
	{
		ChosenEventID = ValidEntries.Last()->EventID;
	}

	return ChosenEventID;
}

bool UEventDistributorComponent::TriggerEventPool(AMyProject1Character* PlayerCharacter, FName EventPoolIDOverride, const TSoftObjectPtr<USkeletalMesh>& ExtraParticipantMeshOverride, bool bHideContextActorDuringAnimEvent)
{
	if (!PlayerCharacter) return false;

	const FName ChosenEventID = RollEventPool(PlayerCharacter, EventPoolIDOverride);
	if (ChosenEventID.IsNone()) return false;

	// 呼び出し側がExtraParticipantMeshOverrideを指定しなかった場合、このコンポーネントのデフォルト値
	// （ANPCSpawner::bOverrideExtraParticipantMeshWithOwnMeshで設定される敵自身のジョブメッシュ等）を使う
	const TSoftObjectPtr<USkeletalMesh>& MeshOverrideToUse = ExtraParticipantMeshOverride.IsNull() ? ExtraParticipantMeshOverrideDefault : ExtraParticipantMeshOverride;

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(PlayerCharacter->GetGameInstance()))
	{
		// GetOwner()：このコンポーネントが付いているNPC/QuestItemPoint/敵Actor。
		// FAnimEventStep::PlayTarget=NPC時の再生対象として使われる（StartEvent参照）
		GameInst->StartEvent(ChosenEventID, PlayerCharacter, GetOwner(), MeshOverrideToUse, bHideContextActorDuringAnimEvent || bHideOwnerDuringAnimEventDefault);
		return true;
	}

	return false;
}
