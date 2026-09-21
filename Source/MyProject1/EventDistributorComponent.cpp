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

void UEventDistributorComponent::TriggerEventPool(AMyProject1Character* PlayerCharacter)
{
	if (!PlayerCharacter || !EventPoolDataTable || EventPoolID.IsNone()) return;

	FEventPoolData* PoolData = EventPoolDataTable->FindRow<FEventPoolData>(EventPoolID, TEXT("TriggerEventPool"));
	if (!PoolData || PoolData->Entries.Num() == 0) return;

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

	if (ValidEntries.Num() == 0 || TotalWeight <= 0.0f) return;

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	FName ChosenEventID = NAME_None;
	for (const FEventPoolEntry* Entry : ValidEntries)
	{
		Roll -= Entry->Weight;
		if (Roll <= 0.0f)
		{
			ChosenEventID = Entry->EventID;
			break;
		}
	}

	// 浮動小数点誤差でどの候補にも当たらなかった場合の保険（末尾候補を採用する）
	if (ChosenEventID.IsNone())
	{
		ChosenEventID = ValidEntries.Last()->EventID;
	}

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(PlayerCharacter->GetGameInstance()))
	{
		// GetOwner()：このコンポーネントが付いているNPC/QuestItemPoint/敵Actor。
		// FAnimEventStep::PlayTarget=NPC時の再生対象として使われる（StartEvent参照）
		GameInst->StartEvent(ChosenEventID, PlayerCharacter, GetOwner());
	}
}
