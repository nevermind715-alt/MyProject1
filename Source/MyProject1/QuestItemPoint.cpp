#include "QuestItemPoint.h"
#include "Components/StaticMeshComponent.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "Kismet/GameplayStatics.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

AQuestItemPoint::AQuestItemPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// TargetNearestEnemy()のターゲット判定はActorのTag（"Enemy"か"NPC"）を見ているだけなので、
	// これを付けておくだけでNPCと同じ射程判定(InteractRange)に自動的に乗る
	Tags.Add(FName("NPC"));
}

void AQuestItemPoint::BeginPlay()
{
	Super::BeginPlay();

	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerChar) return;

	UpdateFlagVisibility(PlayerChar);

	// RequiredFlagが空欄でも購読だけはしておいて問題ない（OnPlayerFlagAdded側で無視される）
	PlayerChar->OnFlagAdded.AddDynamic(this, &AQuestItemPoint::OnPlayerFlagAdded);
}

void AQuestItemPoint::UpdateFlagVisibility(const AMyProject1Character* PlayerChar)
{
	const bool bUnlocked = RequiredFlag.IsNone() || (PlayerChar && PlayerChar->HasFlag(RequiredFlag));

	SetActorHiddenInGame(!bUnlocked);
	SetActorEnableCollision(bUnlocked);
}

void AQuestItemPoint::OnPlayerFlagAdded(FName FlagName)
{
	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}

void AQuestItemPoint::TryInteract(AMyProject1Character* Interactor)
{
	if (!Interactor) return;

	if (bOneTimeUse && bUsed)
	{
		if (!FailureLogText.IsEmpty())
		{
			Interactor->OnReceiveLogMessage(FailureLogText, ELogMessageType::System);
		}
		else
		{
			Interactor->OnReceiveLogMessage(TEXT("もう何も残っていないようだ。"), ELogMessageType::System);
		}
		return;
	}

	bool bSuccess = false;
	switch (Mode)
	{
	case EItemInteractMode::Acquire:
		if (UInventoryComponent* InvComp = Interactor->FindComponentByClass<UInventoryComponent>())
		{
			bSuccess = InvComp->AddItem(ItemID, Amount);
		}
		break;
	case EItemInteractMode::Absorb:
		if (UInventoryComponent* InvComp = Interactor->FindComponentByClass<UInventoryComponent>())
		{
			bSuccess = InvComp->RemoveItem(ItemID, Amount);
		}
		break;
	case EItemInteractMode::InteractOnly:
		// アイテムのやり取りをしないので、インタラクトした時点で常に成功扱い（墓参り・お参り等）
		bSuccess = true;
		break;
	}

	if (!bSuccess)
	{
		if (!FailureLogText.IsEmpty())
		{
			Interactor->OnReceiveLogMessage(FailureLogText, ELogMessageType::System);
		}
		else
		{
			const FString DefaultMsg = (Mode == EItemInteractMode::Absorb) ? TEXT("必要なアイテムが足りない。") : TEXT("これ以上持てない。");
			Interactor->OnReceiveLogMessage(DefaultMsg, ELogMessageType::System);
		}
		return;
	}

	if (!SuccessLogText.IsEmpty())
	{
		Interactor->OnReceiveLogMessage(SuccessLogText, SuccessLogType);
	}

	// 会話クエスト(Delivery型)の「話した」扱いと同じ経路に乗せる。UpdateTalkObjectiveはAActor*とTagsの一致しか見ないため、
	// NPCでなくてもこのアクタ自身をそのまま渡せる
	if (!TalkProgressQuestID.IsNone())
	{
		if (UQuestComponent* QuestComp = Interactor->FindComponentByClass<UQuestComponent>())
		{
			QuestComp->UpdateTalkObjective(TalkProgressQuestID, this);
		}
	}

	if (InteractSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), InteractSound);
	}

	if (bOneTimeUse)
	{
		bUsed = true;
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
