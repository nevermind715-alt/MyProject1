#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyProject1Types.h"
#include "QuestNPCBase.generated.h"

// クエストを提示するNPCの共通C++基底クラス。
// 複数のFQuestDialogSetを優先順位付きで持てるので、1体のNPCが「クエスト1完了後は自動でクエスト2を提示する」
// といった連鎖・分岐を、Blueprint側に判定ロジックを書かずに表現できる。
UCLASS()
class MYPROJECT1_API AQuestNPCBase : public ACharacter
{
	GENERATED_BODY()

public:
	AQuestNPCBase();

	// ログ表示等に使うNPCの名前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	FString NPCName;

	// このNPCが使う会話データテーブル（DT_Dialogs）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	class UDataTable* DialogTable;

	// このNPCが扱うクエストのリスト。配列の先頭ほど優先度が高い。
	// 進行中/報告待ちのクエストがあればそれを継続表示し、無ければ受注条件を満たした先頭の未受注クエストを提示する
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	TArray<FQuestDialogSet> Quests;

	// BPI_InteractableのイベントからEキー等で呼び出す、会話開始の共通ネイティブ処理
	UFUNCTION(BlueprintCallable, Category = "QuestNPC")
	void TalkToNPC(class AMyProject1Character* PlayerChar);
};
