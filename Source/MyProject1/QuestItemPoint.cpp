#include "QuestItemPoint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
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

	// 将来アニメ付きモデルを使う時用。見た目専用なので、当たり判定はMesh側だけに任せる
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 落書き消しクエスト等、平面のデカールだけを見た目として使いたい時用
	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	Decal->SetupAttachment(RootComponent);
	Decal->DecalSize = FVector(64.f, 128.f, 128.f);

	// プレイヤー接近を検知してApproachSoundを鳴らすための専用コリジョン（インタラクト当たり判定とは別）
	ApproachTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ApproachTrigger"));
	ApproachTrigger->SetupAttachment(RootComponent);
	ApproachTrigger->InitSphereRadius(300.f);
	// WallWarpLinkのCollisionBoxと同じ理由（プロジェクト独自のコリジョンチャンネルも確実に拾うため全チャンネルOverlap）
	ApproachTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ApproachTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	ApproachTrigger->SetCollisionResponseToAllChannels(ECR_Overlap);
	ApproachTrigger->SetGenerateOverlapEvents(true);
	ApproachTrigger->OnComponentBeginOverlap.AddDynamic(this, &AQuestItemPoint::OnApproachTriggerBeginOverlap);

	// TargetNearestEnemy()のターゲット判定はActorのTag（"Enemy"か"NPC"）を見ているだけなので、
	// これを付けておくだけでNPCと同じ射程判定(InteractRange)に自動的に乗る
	Tags.Add(FName("NPC"));
}

void AQuestItemPoint::BeginPlay()
{
	Super::BeginPlay();

	InitFlagVisibilityWhenReady();
}

void AQuestItemPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// フラグ初期化待ちのタイマーが回っている途中でアクタが消えた場合の後始末
	GetWorldTimerManager().ClearTimer(FlagInitTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AQuestItemPoint::InitFlagVisibilityWhenReady()
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));

	// プレイヤーがまだ生成されていない（レベル遷移直後・ローディング画面中など）。少し待って再試行
	if (!PlayerChar)
	{
		if (FlagInitAttempts++ < 240) // 0.25秒 * 240 = 最大60秒粘る
		{
			GetWorldTimerManager().SetTimer(FlagInitTimerHandle, this, &AQuestItemPoint::InitFlagVisibilityWhenReady, 0.25f, false);
		}
		return;
	}

	// フラグ通知の購読は一度だけ（同一マップでの「会話→フラグ付与」ライブ経路用）。
	// RequiredFlagが空欄でも購読はしておいて問題ない（OnPlayerFlagAdded側で無視される）。
	// 別レベルからのUnlockedFlags一括復元はAddFlagを経由しないためOnFlagAddedは飛んでこない → 下のHasFlag直接判定で拾う
	if (!bFlagDelegatesBound)
	{
		PlayerChar->OnFlagAdded.AddDynamic(this, &AQuestItemPoint::OnPlayerFlagAdded);
		PlayerChar->OnFlagRemoved.AddDynamic(this, &AQuestItemPoint::OnPlayerFlagRemoved);
		bFlagDelegatesBound = true;
		FlagInitAttempts = 0; // ここからフラグ復元待ちの試行回数を数え直す
	}

	UpdateFlagVisibility(PlayerChar);

	// RequiredFlagもUsedFlagも空欄なら常時表示で確定。以降のリトライは不要
	if (RequiredFlag.IsNone() && UsedFlag.IsNone()) return;

	// プレイヤーは取れたが、判定に使うフラグがまだ復元されていない可能性がある。別レベルからのステータス復元
	// (ApplyPendingCharacterLoad)がこのアクタの初期化より後にずれ込むケースを拾うため、数回だけ再チェックしてから
	// 諦める（以降はOnFlagAdded待ち）
	const bool bAwaitingRequired = !RequiredFlag.IsNone() && !PlayerChar->HasFlag(RequiredFlag);
	const bool bAwaitingUsed     = !UsedFlag.IsNone()     && !PlayerChar->HasFlag(UsedFlag);
	if ((bAwaitingRequired || bAwaitingUsed) && FlagInitAttempts++ < 20) // 0.25秒 * 20 = 5秒
	{
		GetWorldTimerManager().SetTimer(FlagInitTimerHandle, this, &AQuestItemPoint::InitFlagVisibilityWhenReady, 0.25f, false);
	}
}

void AQuestItemPoint::UpdateFlagVisibility(const AMyProject1Character* PlayerChar)
{
	// 使用済みフラグが復元されていれば、RequiredFlagの状態に関わらず消費済みの見た目にして終了
	// （別レベルへ行って戻ってきた時に、bUsedが初期化されて復活してしまうのを防ぐ）
	if (!UsedFlag.IsNone() && PlayerChar && PlayerChar->HasFlag(UsedFlag))
	{
		ApplyUsedState();
		return;
	}

	const bool bUnlocked = RequiredFlag.IsNone() || (PlayerChar && PlayerChar->HasFlag(RequiredFlag));

	SetActorHiddenInGame(!bUnlocked);
	SetActorEnableCollision(bUnlocked);
}

void AQuestItemPoint::OnPlayerFlagAdded(FName FlagName)
{
	// 使用済みフラグが立った（＝このポイントを消費した）なら、以後は消費済みの見た目で固定
	if (!UsedFlag.IsNone() && FlagName == UsedFlag)
	{
		ApplyUsedState();
		return;
	}

	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}

void AQuestItemPoint::OnPlayerFlagRemoved(FName FlagName)
{
	if (RequiredFlag.IsNone() || FlagName != RequiredFlag) return;

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AQuestItemPoint::TryInteract(AMyProject1Character* Interactor)
{
	if (!Interactor) return;

	// 成功・失敗を問わず、インタラクトした時点でターゲットカーソルを消す
	Interactor->CancelTarget();

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

	// ItemIDが空欄なら「アイテムのやり取りはしない」の意味として扱う（お金だけのポイントを作れるようにするため）
	const bool bWantsItem = (Mode != EItemInteractMode::InteractOnly) && !ItemID.IsNone();
	const bool bWantsGil = (Mode != EItemInteractMode::InteractOnly) && GilAmount > 0;

	bool bSuccess = false;
	switch (Mode)
	{
	case EItemInteractMode::Acquire:
		if (UInventoryComponent* InvComp = Interactor->FindComponentByClass<UInventoryComponent>())
		{
			// お金の追加には上限判定がなく必ず成功するので、成否を左右するのはアイテム側の空き容量だけ
			const bool bItemOK = !bWantsItem || InvComp->AddItem(ItemID, Amount);
			if (bItemOK && bWantsGil)
			{
				InvComp->AddGil(GilAmount);
			}
			bSuccess = bItemOK;
		}
		break;
	case EItemInteractMode::Absorb:
		if (UInventoryComponent* InvComp = Interactor->FindComponentByClass<UInventoryComponent>())
		{
			// アイテムとお金、両方使う設定なら、どちらか一方でも足りなければ失敗にする（片方だけ徴収してしまう事故を防ぐ）
			const bool bHasEnoughItem = !bWantsItem || InvComp->GetItemQuantity(ItemID) >= Amount;
			const bool bHasEnoughGil = !bWantsGil || InvComp->Gil >= GilAmount;
			if (bHasEnoughItem && bHasEnoughGil)
			{
				const bool bItemOK = !bWantsItem || InvComp->RemoveItem(ItemID, Amount);
				if (bItemOK && bWantsGil)
				{
					InvComp->TrySpendGil(GilAmount);
				}
				bSuccess = bItemOK;
			}
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
		ApplyUsedState();

		// 使用済みを称号フラグとして永続化する（別レベルへ行って戻ってきても復活しないように）
		if (!UsedFlag.IsNone())
		{
			Interactor->AddFlag(UsedFlag);
		}
	}
}

void AQuestItemPoint::ApplyUsedState()
{
	bUsed = true;
	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetVisibility(false);
	Decal->SetVisibility(false);
}

void AQuestItemPoint::OnApproachTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!ApproachSound) return;

	// 徘徊中のNPCなどが触れても誤発火しないよう、「Player」タグを持つアクターだけを対象にする（WallWarpLinkと同じ判定方式）
	if (!OtherActor || !OtherActor->ActorHasTag(TEXT("Player"))) return;

	if (bApproachSoundOnce && bApproachSoundPlayed) return;

	UGameplayStatics::PlaySound2D(GetWorld(), ApproachSound);
	bApproachSoundPlayed = true;
}
