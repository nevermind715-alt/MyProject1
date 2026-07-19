#include "WBP_QuestSlot.h"
#include "QuestComponent.h"
#include "RpgCharacterInterface.h"
#include "GameFramework/Pawn.h"

void UWBP_QuestSlot::RefreshAchievedMark(FName InQuestID)
{
	bool bAchieved = false;

	// 所有プレイヤーPawnからQuestComponentを取得し、このスロットのクエストが
	// 「条件達成（報告待ち）」状態かどうかを判定する
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		if (IRpgCharacterInterface* CharInterface = Cast<IRpgCharacterInterface>(OwningPawn))
		{
			if (UQuestComponent* QuestComp = CharInterface->GetQuestComponent())
			{
				bAchieved = (QuestComp->GetQuestStatus(InQuestID) == EQuestStatus::ObjectiveCleared);
			}
		}
	}

	OnAchievedMarkChanged(bAchieved);
}
