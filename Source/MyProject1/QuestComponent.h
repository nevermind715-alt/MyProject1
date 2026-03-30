#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // 先ほど追加した構造体を使うためにインクルード
#include "QuestComponent.generated.h"

// UI更新用のデリゲート（クエストが進んだり達成した時にUIに知らせる用）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestUpdated, FName, QuestID);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuestComponent();

	// --- 設定項目 ---
	/** クエストの設計図が書かれたデータテーブル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	UDataTable* QuestDataTable;

	// --- プレイデータ（セーブ対象） ---
	/** 現在進行中のクエスト一覧 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FQuestProgress> ActiveQuests;

	/** すでにクリアしたクエストのID履歴（二重受注を防ぐため） */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FName> CompletedQuests;

	// --- イベント ---
	/** クエストの進行度が変わった時に呼ばれるイベント */
	UPROPERTY(BlueprintAssignable, Category = "Quest|UI")
	FOnQuestUpdated OnQuestUpdated;

	// --- 主要な機能 ---
	/** クエストを受注する */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AcceptQuest(FName QuestID);

	/** クエストを報告して報酬を受け取る */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool ReportQuest(FName QuestID);

	/** 敵を倒した時に呼び出し、討伐クエストのカウントを進める */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateKillObjective(FName EnemyID);

	// --- 便利関数 ---
	/** クエストの状態を確認する（未受注か、進行中か、クリア済か） */
	UFUNCTION(BlueprintPure, Category = "Quest")
	EQuestStatus GetQuestStatus(FName QuestID);

	/** データテーブルからクエスト情報を取得する */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool GetQuestData(FName QuestID, FQuestData& OutData);

	// 追加: アイテムを入手した時にカウントを進める関数
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateGatherObjective(FName ItemID, int32 AmountAdded);

	// 追加: 受注時に、すでに所持しているアイテムをカウントする関数
	void CheckInitialGatherProgress(FName QuestID);
};