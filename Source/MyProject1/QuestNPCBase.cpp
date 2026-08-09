#include "QuestNPCBase.h"
#include "MyProject1Character.h"
#include "QuestComponent.h"
#include "DialogComponent.h"
#include "RpgCharacterInterface.h"

AQuestNPCBase::AQuestNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(FName("NPC"));
}

void AQuestNPCBase::TalkToNPC(AMyProject1Character* PlayerChar)
{
	if (!PlayerChar || !DialogTable || Quests.Num() == 0) return;

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerChar);
	UQuestComponent* QuestComp = RpgInterface ? RpgInterface->GetQuestComponent() : nullptr;
	if (!QuestComp) return;

	TArray<FName> CandidateIDs;
	CandidateIDs.Reserve(Quests.Num());
	for (const FQuestDialogSet& Entry : Quests)
	{
		CandidateIDs.Add(Entry.QuestID);
	}

	// ResolvedQuestIDがNoneになるのは「QuestIDを空欄にした会話専用エントリ」が選ばれた場合もあるため、
	// ここでは弾かない（Quests自体が空の場合は既に上でreturn済み）
	const FName ResolvedQuestID = QuestComp->GetNextOfferableQuest(CandidateIDs);

	const FQuestDialogSet* Entry = Quests.FindByPredicate([&](const FQuestDialogSet& E) { return E.QuestID == ResolvedQuestID; });
	if (!Entry) return;

	FName RowName;

	// 会話クエストで順番を無視して話しかけられた場合は、進捗は進めず、代わりにNotStarted（世間話・挨拶）を出す
	if (!QuestComp->IsExpectedTalkTarget(ResolvedQuestID, this))
	{
		RowName = Entry->DialogRowName_NotStarted;
	}
	else
	{
		switch (QuestComp->GetQuestStatus(ResolvedQuestID))
		{
		case EQuestStatus::NotStarted:       RowName = Entry->DialogRowName_NotStarted; break;
		case EQuestStatus::InProgress:       RowName = Entry->DialogRowName_InProgress; break;
		case EQuestStatus::ObjectiveCleared: RowName = Entry->DialogRowName_ObjectiveCleared; break;
		case EQuestStatus::Completed:        RowName = Entry->DialogRowName_Completed; break;
		}
	}

	if (UDialogComponent* DialogComp = PlayerChar->FindComponentByClass<UDialogComponent>())
	{
		DialogComp->StartDialog(RowName, DialogTable, this);

		// 会話中はプレイヤー操作をロックする（DialogComponent::CloseDialogが解除する処理と対）
		RpgInterface->SetInputLocked(true);
	}
}
