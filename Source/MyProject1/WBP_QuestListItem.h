#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_QuestListItem.generated.h"

UCLASS()
class MYPROJECT1_API UWBP_QuestListItem : public UUserWidget
{
	GENERATED_BODY()

public:
	// このリスト項目が表示しているクエストの「達成済み（報告待ち）」マークを更新する。
	// BP側のConstruct時、およびQuestComponent::OnQuestUpdated発火時に呼び出す想定。
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void RefreshAchievedMark(FName InQuestID);

	// デバッグ用: Ctrlキーを押しながらこの行がクリックされた時に呼ぶ想定。
	// Ctrl未使用時や通常クリック時は内部で何もせずfalseを返すだけなので、
	// OnClicked等の既存の処理と並列に1本つなぐだけで良い（外せば無効化できる）。
	UFUNCTION(BlueprintCallable, Category = "Quest|Debug")
	bool Debug_TryForceCompleteQuest(FName InQuestID);

protected:
	// BP側で達成マーク用ウィジェット（Txt_Achieved）のVisibilityを切り替える
	UFUNCTION(BlueprintImplementableEvent, Category = "Quest")
	void OnAchievedMarkChanged(bool bAchieved);
};
