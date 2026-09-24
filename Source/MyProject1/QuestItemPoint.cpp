#include "QuestItemPoint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EventDistributorComponent.h"
#include "GameplayActionLibrary.h"
#include "RpgCharacterInterface.h"
#include "MyProject1GameInstance.h"

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

	// 成功率つき追加抽選（情報収集ポイント等）。通常の成功判定を通った後にもう一段判定する
	if (bUseChanceOutcome)
	{
		IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(Interactor);
		const bool bChanceSuccess = FMath::FRandRange(0.0f, 100.0f) <= ChanceSuccessPercent;

		if (bChanceSuccess)
		{
			if (RpgInterface)
			{
				for (const FEventAction& Action : ChanceSuccessActions)
				{
					UGameplayActionLibrary::ExecuteAction(RpgInterface, Interactor, this, GetWorld(), Action.ActionType, Action.ActionPayload, Action.ItemID, Action.ItemAmount);
					UGameplayActionLibrary::ApplyStatChange(RpgInterface, this, Action.StatToChange, Action.StatTargetActor, Action.ExtraStatName, Action.StatChangeAmount);
				}
			}
		}
		else
		{
			if (bResolveActiveEventOnChanceFailure)
			{
				if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
				{
					GameInst->ResolveActiveEvent(false);
				}
			}
			else if (RpgInterface)
			{
				for (const FEventAction& Action : ChanceFailureActions)
				{
					UGameplayActionLibrary::ExecuteAction(RpgInterface, Interactor, this, GetWorld(), Action.ActionType, Action.ActionPayload, Action.ItemID, Action.ItemAmount);
					UGameplayActionLibrary::ApplyStatChange(RpgInterface, this, Action.StatToChange, Action.StatTargetActor, Action.ExtraStatName, Action.StatChangeAmount);
				}
			}

			// 追加抽選に失敗した場合はここで終了し、この下の成功ログ・クエスト連携・使用済み処理は行わない
			return;
		}
	}

	// AnimEventIDが設定されていれば、この下の成功後処理（ログ・クエスト連携・イベント分岐・効果音・使用済み化）は
	// アニメーション再生完了後（OnInteractAnimEventFinished）まで遅延する
	if (!AnimEventID.IsNone())
	{
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
		{
			PendingAnimEventInteractor = Interactor;
			bWaitingForAnimEventCompletion = true;
			GameInst->OnAnimSequenceEventFinished.AddUniqueDynamic(this, &AQuestItemPoint::OnInteractAnimEventFinished);
			// ワープを経由しない直接呼び出しのため、開始前にも暗転を挟む（GameplayActionLibraryのPlayAnimSequenceと同じ理由）
			GameInst->PlayAnimSequenceEvent(AnimEventID, Interactor, this, true);
			return;
		}
	}

	FinalizeInteractSuccess(Interactor);
}

void AQuestItemPoint::OnInteractAnimEventFinished(bool bCompletedNormally)
{
	// 他のアクター（NPCとの会話等）が開始したアニメーションイベントの完了通知を誤って拾わないためのガード
	if (!bWaitingForAnimEventCompletion) return;

	bWaitingForAnimEventCompletion = false;

	// bCompletedNormally=false（AnimEventID不正・対象不備等で再生自体が始まらなかった）の場合も、
	// アイテムの授受は既に完了しているためインタラクト自体は成功扱いのまま後処理を進める
	if (AMyProject1Character* Interactor = PendingAnimEventInteractor.Get())
	{
		FinalizeInteractSuccess(Interactor);
	}
	PendingAnimEventInteractor = nullptr;
}

void AQuestItemPoint::FinalizeInteractSuccess(AMyProject1Character* Interactor)
{
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

	// イベント分岐システム：このポイント自身にUEventDistributorComponentが付いていれば、インタラクト成功時に抽選を開始する
	if (UEventDistributorComponent* EventComp = FindComponentByClass<UEventDistributorComponent>())
	{
		EventComp->TriggerEventPool(Interactor);
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

	// Actor自体もHidden化しないと、TargetNearestEnemy/CycleTargetのIsHidden()判定を素通りして
	// 見た目は消えたのにターゲットだけできてしまう（UpdateFlagVisibilityのRequiredFlag非表示と同じ扱いに揃える）
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
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
