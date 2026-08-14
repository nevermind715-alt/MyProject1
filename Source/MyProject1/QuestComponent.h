#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // ��قǒǉ������\���̂��g�����߂ɃC���N���[�h
#include "QuestComponent.generated.h"

// UI�X�V�p�̃f���Q�[�g�i�N�G�X�g���i�񂾂�B����������UI�ɒm�点��p�j
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestUpdated, FName, QuestID);

class AMyProject1HUD;
class AActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UQuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuestComponent();

	// --- �ݒ荀�� ---
	/** �N�G�X�g�̐݌v�}�������ꂽ�f�[�^�e�[�u�� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	UDataTable* QuestDataTable;

	// --- �v���C�f�[�^�i�Z�[�u�Ώہj ---
	/** ���ݐi�s���̃N�G�X�g�ꗗ */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FQuestProgress> ActiveQuests;

	/** ���łɃN���A�����N�G�X�g��ID�����i��d�󒍂�h�����߁j */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FCompletedQuestInfo> CompletedQuests;

	/** リピート可能クエストのクールタイム明けでCompletedQuestsから消えても、実績クエスト判定用に「一度でもクリアしたか」を永続保持する */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FName> EverCompletedQuestIDs;

	// --- �C�x���g ---
	/** �N�G�X�g�̐i�s�x���ς�������ɌĂ΂��C�x���g */
	UPROPERTY(BlueprintAssignable, Category = "Quest|UI")
	FOnQuestUpdated OnQuestUpdated;

	// --- ��v�ȋ@�\ ---
	/** �N�G�X�g���󒍂��� */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AcceptQuest(FName QuestID);

	/** �N�G�X�g��񍐂��ĕ�V���󂯎�� */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool ReportQuest(FName QuestID);

	/** 進行中のクエストを放棄する（あきらめる）。ActiveQuestsから取り除くだけで、報酬・アイテムの巻き戻しは行わない */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CancelQuest(FName QuestID);

	/** デバッグ用: Ctrlキーが押されている時だけ動作し、対象クエストを強制的に条件達成扱いにしてから報告処理まで行う（受注クエスト一覧のCtrl+クリックから呼ぶ想定。呼び出し元のノードを外せば無効化できる） */
	UFUNCTION(BlueprintCallable, Category = "Quest|Debug")
	bool Debug_ForceCompleteQuest(FName QuestID);

	/** �G��|�������ɌĂяo���A�����N�G�X�g�̃J�E���g��i�߂� */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateKillObjective(FName EnemyID);

	/** お使い・会話クエスト（Delivery）で、指定NPCと話した時に呼び出し、順番通りなら進捗を進める（NPCの識別はActorのTagsで行う） */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateTalkObjective(FName QuestID, AActor* TalkedToNPC);

	/** Delivery（会話）クエストがInProgressの場合に、指定NPCが「今まさに話しかけられるべき相手」かどうかを判定する。順番を無視して話しかけたNPCでは会話を出さないようにするための事前チェック用（Delivery以外・進行中でない場合は常にtrue） */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool IsExpectedTalkTarget(FName QuestID, AActor* NPC);

	// --- �֗��֐� ---
	/** �N�G�X�g�̏�Ԃ��m�F����i���󒍂��A�i�s�����A�N���A�ς��j */
	UFUNCTION(BlueprintPure, Category = "Quest")
	EQuestStatus GetQuestStatus(FName QuestID);

	/** �f�[�^�e�[�u������N�G�X�g�����擾���� */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool GetQuestData(FName QuestID, FQuestData& OutData);

	/** 受注条件（RequiredFlag・PrerequisiteQuestIDs・RequiredStats）を全て満たしているか判定する */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CanAcceptQuest(FName QuestID);

	/** 優先順位付きの候補クエストIDリストから、NPCが今提示すべき1件を選ぶ（複数クエスト・連鎖対応NPC用） */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	FName GetNextOfferableQuest(const TArray<FName>& CandidateQuestIDs);

	// �ǉ�: �A�C�e������肵�����ɃJ�E���g��i�߂�֐�
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateGatherObjective(FName ItemID, int32 AmountAdded);

	// �ǉ�: �󒍎��ɁA���łɏ������Ă���A�C�e�����J�E���g����֐�
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