#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h"
#include "ChestComponent.generated.h"

// UI���Łu�`�F�X�g�̒��g���ω������v���Ƃ����m���邽�߂̃f���Q�[�g
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestUpdated);

// BP_Chest側に「今から蓋を開閉する」ことを知らせるためのデリゲート（実際の見た目はBP側のTimelineが担当）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestOpenRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestCloseRequested);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UChestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChestComponent();

	// ���g���ω���������UI�ɒm�点�鍇�}
	UPROPERTY(BlueprintAssignable, Category = "Chest|UI")
	FOnChestUpdated OnChestUpdated;

protected:
	virtual void BeginPlay() override;

public:
	// ==========================================
	// 表示名（WBP_NPCNameに渡す用）
	// ==========================================

	/** ボックスコリジョンにオーバーラップした時、WBP_NPCNameに表示する名前 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Display")
	FString ChestName = TEXT("宝箱");

	// ==========================================
	// �ݒ荀�� (�G�f�B�^�̃v���p�e�B�Őݒ�)
	// ==========================================

	/** True�Ȃ�f�[�^�e�[�u������S�A�C�e���������������Ċi�[����i�f�o�b�O�p�j */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bGenerateAllItems = false;

	/** �f�o�b�O�����������ɁA�e�A�C�e�����������ɓ���邩 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings", meta = (EditCondition = "bGenerateAllItems"))
	int32 DebugItemQuantity = 99;

	/** True�Ȃ牽�x���o���Ă����g������Ȃ��i�������j */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bIsInfinite = false;

	/** True�Ȃ璆�g����ɂȂ������ɐeActor�i�`�F�X�g���́j�����[���h����폜���� */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bDestroyWhenEmpty = false;

	/** �ǂݍ��ރA�C�e�����X�g�̃f�[�^�e�[�u���iDT_Items�Ȃǂ��Z�b�g����j */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings", meta = (EditCondition = "bGenerateAllItems"))
	UDataTable* ItemDataTable;

	// ==========================================
	// �f�[�^ (���݂̒��g)
	// ==========================================

	/** �`�F�X�g�̒��g�BbGenerateAllItems��False�̏ꍇ�́A�蓮�ŃA�C�e����ݒ肵�܂��B */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Content")
	TArray<FInventorySlot> ChestContents;

	// ==========================================
	// �@�\
	// ==========================================

	/** �v���C���[���`�F�X�g�������̃A�C�e�������o������ */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	bool TakeItem(FName ItemID, int32 RequestAmount, class AMyProject1Character* InteractingPlayer);

	/** ���݂̃`�F�X�g�̒��g���擾����iUI�\���p�j */
	UFUNCTION(BlueprintPure, Category = "Chest")
	TArray<FInventorySlot> GetChestContents() const { return ChestContents; }

	// ==========================================
	// �J���J�i�~�߁j��ԁ{E�L�[�C���^���N�g
	// ==========================================

	/** ���ɊJ���Ă��邩�i��d�A�j���[�V�����h�~�Ǝ��C���^���N�g���̕���Ɏg���j */
	UPROPERTY(BlueprintReadOnly, Category = "Chest|State")
	bool bIsOpen = false;

	/** BPI_Interactable��E�L�[�C�x���g����Ăяo���A�B��̓�����i�J��+HUD�\����ʊ��j */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	void Interact(class AMyProject1Character* InteractingPlayer);

	/** BPI_Targetable��bIsTargeted=false�i�J�[�\�������j����Ăяo���A�~�߂鏈�� */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	void CloseChest();

	// ==========================================
	// 蓋の見た目（実際の回転演出）はBP_Chest側のTimelineに任せる。
	// C++は「いつ再生するか」だけをデリゲートで合図として送る。
	// ==========================================

	/** 初回インタラクト時にブロードキャストされる。BP_Chest側でこれをバインドし、Timelineを順再生する */
	UPROPERTY(BlueprintAssignable, Category = "Chest|Animation")
	FOnChestOpenRequested OnChestOpenRequested;

	/** ターゲットカーソルが消えた時にブロードキャストされる。BP_Chest側でこれをバインドし、Timelineを逆再生する */
	UPROPERTY(BlueprintAssignable, Category = "Chest|Animation")
	FOnChestCloseRequested OnChestCloseRequested;
};