#include "EventDistributorComponent.h"
#include "MyProject1Character.h"
#include "MyProject1GameInstance.h"
#include "DialogComponent.h"
#include "RpgCharacterInterface.h"
#include "GameplayActionLibrary.h"
#include "Misc/ScopeExit.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

UEventDistributorComponent::UEventDistributorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FName UEventDistributorComponent::RollEventPool(AMyProject1Character* PlayerCharacter, FName EventPoolIDOverride) const
{
	return RollEventPoolFromTable(PlayerCharacter, EventPoolDataTable, EventPoolIDOverride.IsNone() ? EventPoolID : EventPoolIDOverride);
}

FName UEventDistributorComponent::RollEventPoolFromTable(AMyProject1Character* PlayerCharacter, const UDataTable* PoolTable, FName PoolIDToUse)
{
	if (!PlayerCharacter || !PoolTable || PoolIDToUse.IsNone()) return NAME_None;

	FEventPoolData* PoolData = PoolTable->FindRow<FEventPoolData>(PoolIDToUse, TEXT("RollEventPool"));
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

	// DefeatDialogRowの会話経由の呼び出しなら、抽選結果（何も起きない場合を含む）にかかわらず、
	// 通常の敗北経路（AMyProject1Character::TakeDamage）と同じ後始末をここで行う
	const bool bFinishDefeat = bDefeatCleanupPending;
	bDefeatCleanupPending = false;
	ON_SCOPE_EXIT
	{
		if (bFinishDefeat)
		{
			OnDefeatedPlayer.Broadcast();
		}
	};

	const FName ChosenEventID = RollEventPool(PlayerCharacter, EventPoolIDOverride);
	UE_LOG(LogTemp, Log, TEXT("[EventPool] TriggerEventPool owner=%s pool=%s chosen=%s defeatFinish=%d"),
		*GetNameSafe(GetOwner()), *(EventPoolIDOverride.IsNone() ? EventPoolID : EventPoolIDOverride).ToString(), *ChosenEventID.ToString(), bFinishDefeat ? 1 : 0);
	if (ChosenEventID.IsNone()) return false;

	// 呼び出し側がExtraParticipantMeshOverrideを指定しなかった場合、このコンポーネントのデフォルト値
	// （ANPCSpawner::bOverrideExtraParticipantMeshWithOwnMeshで設定される敵自身のジョブメッシュ等）を使う
	const TSoftObjectPtr<USkeletalMesh>& MeshOverrideToUse = ExtraParticipantMeshOverride.IsNull() ? ExtraParticipantMeshOverrideDefault : ExtraParticipantMeshOverride;

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(PlayerCharacter->GetGameInstance()))
	{
		// GetOwner()：このコンポーネントが付いているNPC/QuestItemPoint/敵Actor。
		// FAnimEventStep::PlayTarget=NPC時の再生対象として使われる（StartEvent参照）
		// 戦闘敗北経路と同じく、イベントが実際に始まる場合は倒れたままにならないよう全快させる。
		// 暗転セリフの追加アクションから呼ばれた場合、StartEventの中で（既に真っ暗なため）その場でワープまで進み、
		// 別レベルへのワープのスナップショットも取られる。全快はそれより前でないと瀕死のまま保存されるので、StartEventの前に行う
		if (bFinishDefeat)
		{
			PlayerCharacter->Revive(PlayerCharacter->MyStats.MaxHP);
			PlayerCharacter->MyStats.Stamina = PlayerCharacter->MyStats.MaxStamina;
			if (PlayerCharacter->OnStaminaChangedDelegate.IsBound())
			{
				PlayerCharacter->OnStaminaChangedDelegate.Broadcast(PlayerCharacter->MyStats.Stamina, PlayerCharacter->MyStats.MaxStamina);
			}
		}

		GameInst->StartEvent(ChosenEventID, PlayerCharacter, GetOwner(), MeshOverrideToUse, bHideContextActorDuringAnimEvent || bHideOwnerDuringAnimEventDefault);
		return true;
	}

	return false;
}

bool UEventDistributorComponent::IsEventDistributorStatConditionMet(AMyProject1Character* PlayerCharacter) const
{
	if (!EventDistributorStatCondition.bUseStatCondition) return true;

	float StatValue = 0.0f;
	if (!UGameplayActionLibrary::TryGetTargetStatValue(PlayerCharacter, GetOwner(), EventDistributorStatCondition.RequiredStat,
		EStatTargetActor::Player, EventDistributorStatCondition.RequiredStatExtraName, StatValue))
	{
		return false;
	}

	const float RequiredValue = EventDistributorStatCondition.RequiredStatValue;
	switch (EventDistributorStatCondition.RequiredStatCompareOp)
	{
	case EStatCompareOp::GreaterOrEqual: return StatValue >= RequiredValue;
	case EStatCompareOp::LessOrEqual:    return StatValue <= RequiredValue;
	case EStatCompareOp::Equal:          return FMath::IsNearlyEqual(StatValue, RequiredValue);
	case EStatCompareOp::Greater:        return StatValue > RequiredValue;
	case EStatCompareOp::Less:           return StatValue < RequiredValue;
	}
	return false;
}

bool UEventDistributorComponent::TryStartDefeatDialog(AMyProject1Character* PlayerCharacter)
{
	if (!PlayerCharacter || !DefeatDialogRow.DataTable || DefeatDialogRow.RowName.IsNone()) return false;

	UDialogComponent* DialogComp = PlayerCharacter->FindComponentByClass<UDialogComponent>();
	if (!DialogComp) return false;

	// 会話の中でTriggerEventPoolが呼ばれるまでの後始末の保留。始められなかった場合は下で戻す
	bDefeatCleanupPending = true;
	UE_LOG(LogTemp, Log, TEXT("[EventPool] 敗北会話を開始: owner=%s row=%s"), *GetNameSafe(GetOwner()), *DefeatDialogRow.RowName.ToString());
	// FDataTableRowHandleのDataTableはconst。TryStartDialogは非constを要求するが、テーブルは読み取りしかしない
	if (!DialogComp->TryStartDialog(DefeatDialogRow.RowName, const_cast<UDataTable*>(DefeatDialogRow.DataTable.Get()), GetOwner()))
	{
		bDefeatCleanupPending = false;
		return false;
	}
	return true;
}
