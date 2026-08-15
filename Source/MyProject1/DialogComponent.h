#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // ��قǍ�����\���̂��g������
#include "DialogComponent.generated.h"

// UI�ɒʒm���邽�߂̃f���Q�[�g�i�Z���t�f�[�^�ƁA�����Ă���NPC�̏��𑗂�j
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogUpdated, const FDialogData&, DialogData, AActor*, NPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideChoices);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UDialogComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogComponent();

	// ��b���J�n����֐��i�f�[�^�e�[�u���̍s�����w��j
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void StartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC);

	// �I�������I�΂ꂽ����UI����Ă΂��֐�
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void SelectChoice(int32 ChoiceIndex);

	// --- ���̑I�����̏����i���_�l�E�t���O�j�𖞂����Ă��邩�`�F�b�N���� ---
	UFUNCTION(BlueprintPure, Category = "Dialog")
	bool CanSelectChoice(const FDialogChoice& Choice) const;

	// �I�������Ȃ���b�ŁA��ʂ��N���b�N���āu���֐i�ށv���ɌĂ΂��֐� ---
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void AdvanceDialog();

	// ��b�������I������
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void CloseDialog();

	// UI���ł���ɃC�x���g���o�C���h����
	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnDialogUpdated OnDialogUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnDialogClosed OnDialogClosed;

	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnHideChoices OnHideChoices;

private:
	// ���ݎg�p���̃f�[�^�e�[�u����NPC
	UPROPERTY()
	UDataTable* CurrentTable;

	UPROPERTY()
	AActor* CurrentNPC;

	// ���݂̉�b�f�[�^
	FDialogData CurrentDialogData;

	// アクションの実行本体（Choice経由でもセリフ単体経由でも共通で使う）
	void ExecuteActionCore(EDialogActionType ActionType, const FString& ActionPayload, FName GrantFlag, bool bFadeOnGrantFlag, FName FlagToRemove, bool bFadeOnRemoveFlag, ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount);

	// --- ���s�����V�X�e���p�̕ϐ��Ɗ֐� ---

	/** �������ꂽ�e�L�X�g��ێ�����z�� */
	UPROPERTY()
	TArray<FString> CurrentDialogLines;

	/** ���݉��s�ڂ�\�����Ă��邩�̃C���f�b�N�X */
	int32 CurrentLineIndex = 0;

	/** ���݂̍s�̃e�L�X�g��UI�֑��M����֐� */
	void ShowCurrentLine();
};