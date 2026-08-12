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

protected:
	virtual void BeginPlay() override;

public:
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

	// --- フラグによる表示/非表示切り替え ---
	// 例：「元の場所のNPC」にHideOnFlag、「移動先に先置きした同じNPC」にShowOnFlagを
	// 同じフラグ名で設定しておくと、会話でフラグが立った瞬間に入れ替わったように見える
	/** 表示切り替えの判定に使うフラグ（空欄、またはVisibilityModeがNoneなら常に表示のまま） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Flag Visibility")
	FName VisibilityFlag;

	/** VisibilityFlagの状態に応じて、表示するか消えるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Flag Visibility")
	EFlagVisibilityMode VisibilityMode = EFlagVisibilityMode::None;

private:
	/** VisibilityFlag/VisibilityModeの設定に応じて、現在の表示/当たり判定を更新する */
	void UpdateFlagVisibility(const class AMyProject1Character* PlayerChar);

	/** プレイヤーがフラグを獲得した時に呼ばれる（OnFlagAddedの購読先）。VisibilityFlag一致時のみ表示を更新する */
	UFUNCTION()
	void OnPlayerFlagAdded(FName FlagName);
};
