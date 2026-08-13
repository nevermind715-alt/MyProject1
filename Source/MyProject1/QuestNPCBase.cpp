#include "QuestNPCBase.h"
#include "MyProject1Character.h"
#include "QuestComponent.h"
#include "DialogComponent.h"
#include "RpgCharacterInterface.h"
#include "Kismet/GameplayStatics.h"

AQuestNPCBase::AQuestNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(FName("NPC"));
}

void AQuestNPCBase::BeginPlay()
{
	Super::BeginPlay();

	if (VisibilityMode == EFlagVisibilityMode::None || VisibilityFlag.IsNone()) return;

	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerChar) return;

	UpdateFlagVisibility(PlayerChar);
	PlayerChar->OnFlagAdded.AddDynamic(this, &AQuestNPCBase::OnPlayerFlagAdded);
	PlayerChar->OnFlagRemoved.AddDynamic(this, &AQuestNPCBase::OnPlayerFlagRemoved);
}

void AQuestNPCBase::UpdateFlagVisibility(const AMyProject1Character* PlayerChar)
{
	const bool bHasFlag = PlayerChar && PlayerChar->HasFlag(VisibilityFlag);
	ApplyVisibilityForFlagState(bHasFlag);
}

void AQuestNPCBase::ApplyVisibilityForFlagState(bool bHasFlag)
{
	const bool bVisible = (VisibilityMode == EFlagVisibilityMode::ShowOnFlag) ? bHasFlag : !bHasFlag;
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);
}

void AQuestNPCBase::OnPlayerFlagAdded(FName FlagName)
{
	if (VisibilityFlag.IsNone() || FlagName != VisibilityFlag) return;
	ApplyVisibilityForFlagState(true);
}

void AQuestNPCBase::OnPlayerFlagRemoved(FName FlagName)
{
	if (VisibilityFlag.IsNone() || FlagName != VisibilityFlag) return;
	ApplyVisibilityForFlagState(false);
}

void AQuestNPCBase::TalkToNPC(AMyProject1Character* PlayerChar)
{
	if (!PlayerChar || !DialogTable) return;

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerChar);
	if (!RpgInterface) return;

	// 初回だけの挨拶（フラグ未取得の間だけ表示。Quests配列より優先。
	// フラグの付与は自分では行わず、DT_Dialogs側の対象行のGrantFlagに任せる）
	if (!FirstMeetFlag.IsNone() && !RpgInterface->HasFlag(FirstMeetFlag))
	{
		if (UDialogComponent* DialogComp = PlayerChar->FindComponentByClass<UDialogComponent>())
		{
			DialogComp->StartDialog(DialogRowName_FirstMeet, DialogTable, this);
			RpgInterface->SetInputLocked(true);
		}
		return;
	}

	if (Quests.Num() == 0) return;

	UQuestComponent* QuestComp = RpgInterface->GetQuestComponent();
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
		case EQuestStatus::NotStarted:
			// 受注条件（フラグ・前提クエスト等）を満たすまでは、クエストの気配を出さないLocked行を表示する。
			// Locked行が未設定のNPCは従来通りNotStarted行をそのまま使う
			if (!QuestComp->CanAcceptQuest(ResolvedQuestID) && !Entry->DialogRowName_Locked.IsNone())
			{
				RowName = Entry->DialogRowName_Locked;
			}
			else
			{
				RowName = Entry->DialogRowName_NotStarted;
			}
			break;
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
