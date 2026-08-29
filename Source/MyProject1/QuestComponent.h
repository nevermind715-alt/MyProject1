#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // 後ほど追加した構造体を使うためにインクルード
#include "QuestComponent.generated.h"

// UI更新用のデリゲート（クエストが進んだ・完了したことをUIに知らせる用）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestUpdated, FName, QuestID);

class AMyProject1HUD;
class AActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuestComponent();

	// --- 設定項目 ---
	/** クエストの設計図が入れられたデータテーブル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	UDataTable* QuestDataTable;

	// --- プレイデータ（セーブ対象） ---
	/** 現在進行中のクエスト一覧 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FQuestProgress> ActiveQuests;

	/** すでにクリアしたクエストのIDたち（二重受注を防ぐため） */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FCompletedQuestInfo> CompletedQuests;

	/** リピート可能クエストのクールタイム明けでCompletedQuestsから消えても、実績クエスト判定用に「一度でもクリアしたか」を永続保持する */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FName> EverCompletedQuestIDs;

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

	/** 進行中のクエストを放棄する（あきらめる）。ActiveQuestsから取り除くだけで、報酬・アイテムの巻き戻しは行わない */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CancelQuest(FName QuestID);

	/** デバッグ用: Ctrlキーが押されている時だけ動作し、対象クエストを強制的に条件達成扱いにしてから報告処理まで行う（受注クエスト一覧のCtrl+クリックから呼ぶ想定。呼び出し元のノードを外せば無効化できる） */
	UFUNCTION(BlueprintCallable, Category = "Quest|Debug")
	bool Debug_ForceCompleteQuest(FName QuestID);

	/** 敵を倒した時に呼び出され、討伐クエストのカウントを進める */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateKillObjective(FName EnemyID);

	/** お使い・会話クエスト（Delivery）で、指定NPCと話した時に呼び出し、順番通りなら進捗を進める（NPCの識別はActorのTagsで行う） */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateTalkObjective(FName QuestID, AActor* TalkedToNPC);

	/** Delivery（会話）クエストがInProgressの場合に、指定NPCが「今まさに話しかけられるべき相手」かどうかを判定する。順番を無視して話しかけたNPCでは会話を出さないようにするための事前チェック用（Delivery以外・進行中でない場合は常にtrue） */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool IsExpectedTalkTarget(FName QuestID, AActor* NPC);

	// --- 便利関数 ---
	/** クエストの状態を確認する（未受注・進行中・クリア済み） */
	UFUNCTION(BlueprintPure, Category = "Quest")
	EQuestStatus GetQuestStatus(FName QuestID);

	/** データテーブルからクエスト情報を取得する */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool GetQuestData(FName QuestID, FQuestData& OutData);

	/** 受注条件（RequiredFlag・PrerequisiteQuestIDs・RequiredStats）を全て満たしているか判定する */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CanAcceptQuest(FName QuestID);

	/** CanAcceptQuestと同じ判定を行い、受注できない場合はその理由（クールダウン中／条件未達等）をOutFailReasonに格納する（ログ表示用、C++専用） */
	bool CanAcceptQuest(FName QuestID, FString& OutFailReason);

	/** 優先順位付きの候補クエストIDリストから、NPCが今提示すべき1件を選ぶ（複数クエスト・連鎖対応NPC用） */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	FName GetNextOfferableQuest(const TArray<FName>& CandidateQuestIDs);

	// 追加: アイテムを入手した時にカウントを進める関数
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateGatherObjective(FName ItemID, int32 AmountAdded);

	// 追加: 受注時に、すでに所持しているアイテムをカウントする関数
	void CheckInitialGatherProgress(FName QuestID);

	/** 前提クエストが1つクリアされるたびに呼ばれ、進行中の実績クエストの達成状況を更新する */
	void UpdateAchievementObjective(FName CompletedQuestID);

	/** 実績クエストを受注した直後、すでにクリア済みの前提クエストがあれば即座に進捗へ反映する */
	void CheckInitialAchievementProgress(FName QuestID);

private:
	// Sound resolution helpers: DT_QuestData row overrides HUD default sound when set
	AMyProject1HUD* GetOwnerHUD() const;
	class USoundBase* ResolveQuestSound(class USoundBase* RowSound, class USoundBase* AMyProject1HUD::* DefaultField) const;
};
