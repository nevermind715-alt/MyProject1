#include "WarpPortal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "MyProject1GameInstance.h"

// ���{�ꕶ�������E�R���p�C���G���[�΍�
#pragma execution_character_set("utf-8")

AWarpPortal::AWarpPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	// �����蔻��̃{�b�N�X���쐬
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	// ���̑傫����ݒ�i�����傫�߂Ɂj
	CollisionBox->InitBoxExtent(FVector(100.f, 100.f, 100.f));

	// �Փːݒ�i�v���C���[�݂̂ɔ���������̂����z�ł����A�����ł͈�ʓI�Ȑݒ�Ɂj
	CollisionBox->SetCollisionProfileName(TEXT("Trigger"));

	// �G�ꂽ���̃C�x���g��o�^
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWarpPortal::OnOverlapBegin);
}

void AWarpPortal::BeginPlay()
{
	Super::BeginPlay();
}

void AWarpPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// �A�N�^���L�����N�^�[�i�v���C���[�j���ǂ����`�F�b�N�i�p�g���[����NPC�Ȃǂ��Փ˂��Ĉ�Ď��s���Ȃ��悤�A
	// �uPlayer�v�^�O�������Ă��邩���K���m�F����j
	if (!OtherActor->ActorHasTag(TEXT("Player"))) return;

	if (ACharacter* PlayerChar = Cast<ACharacter>(OtherActor))
	{
		// �u�G���Ƒ����[�v�v�̐ݒ�Ȃ�A�����ɔ��
		if (PortalType == EWarpPortalType::TouchToWarp)
		{
			ExecuteWarp(PlayerChar);
		}
		// ��InteractToWarp�i���ׂ�j�� ConfirmToWarp �̏ꍇ�͂����ł͉������Ȃ��iBP�����InteractWithPortal��҂j
	}
}

void AWarpPortal::InteractWithPortal(ACharacter* Interactor)
{
	// E�L�[���Œ��ׂ�ꂽ��
	if (PortalType == EWarpPortalType::InteractToWarp)
	{
		ExecuteWarp(Interactor);
	}
}

void AWarpPortal::ExecuteWarp(ACharacter* TargetCharacter)
{
	if (!TargetCharacter || TargetWarpID.IsNone()) return;

	// ���x�z�l�iGameInstance�j���Ă�Ń��[�v���˗�����
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInst->RequestWarp(TargetWarpID, TargetCharacter);
	}
}