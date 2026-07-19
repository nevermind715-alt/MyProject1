#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_QuestSlot.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_QuestSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	// このクエストスロットが表示しているクエストの「達成済み（報告待ち）」マークを更新する。
	// BP側のConstruct時、およびQuestComponent::OnQuestUpdated発火時に呼び出す想定。
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void RefreshAchievedMark(FName InQuestID);

protected:
	// BP側で達成マーク用ウィジェット（TextBlock等）のVisibilityを切り替える
	UFUNCTION(BlueprintImplementableEvent, Category = "Quest")
	void OnAchievedMarkChanged(bool bAchieved);
};
