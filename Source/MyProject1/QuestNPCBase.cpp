#include "QuestNPCBase.h"
#include "MyProject1Character.h"
#include "SkinOverlayComponent.h"
#include "MyAIController.h"
#include "QuestComponent.h"
#include "DialogComponent.h"
#include "InventoryComponent.h"
#include "MyProject1GameInstance.h"
#include "RpgCharacterInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

AQuestNPCBase::AQuestNPCBase()
{
	Tags.Add(FName("NPC"));

	// AMyAIController自体はACharacter::PossessedByで自動Possessされないため、
	// AutoPossessAIとAIControllerClassを設定して初めて徘徊AIが動き出す
	// （bCanPatrol=falseの個体は、AIは走るがBT側でCanPatrolを見て静止したままになる）
	AIControllerClass = AMyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// AMyProject1Character由来のCameraBoom/FollowCameraはNPCでは使わないが、
	// 継承の都合上どうしても付いてくる。非アクティブ化しておくことで、エディタでNPCアクターを選択した際に
	// レベルビューポート右下に自動で出るカメラプレビュー（FLevelEditorViewportClient::FindViewComponentForActorが
	// Comp->IsActive()を条件に対象コンポーネントを探す仕様のため）を防ぐ
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		Boom->Deactivate();
		Boom->bAutoActivate = false;
	}
	if (UCameraComponent* Cam = GetFollowCamera())
	{
		Cam->Deactivate();
		Cam->bAutoActivate = false;
	}

#if WITH_EDITORONLY_DATA
	// ゲーム動作には影響しないエディタ表示だけを消す。
	// bVisualizeComponent=falseはコンポーネントのアイコン表示用のフラグで、CameraComponent::OnRegister()が
	// 生成する「カメラ形状のメッシュ(ProxyMeshComponent)」と「視錐台(DrawFrustum)」には効かないため、
	// 複数NPCが重なって邪魔になっていたのはこの2つ。SetCameraMesh(nullptr)とbDrawFrustumAllowed=falseで消す
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		Boom->bVisualizeComponent = false;
	}
	if (UCameraComponent* Cam = GetFollowCamera())
	{
		Cam->bVisualizeComponent = false;
		Cam->SetCameraMesh(nullptr);
		Cam->bDrawFrustumAllowed = false;
	}
#endif
}

void AQuestNPCBase::BeginPlay()
{
	Super::BeginPlay();

	// Super::BeginPlay()のApplyJobData()でベース体型（本体メッシュ・髪・武器等）が入った後に、
	// DT_Equipments側の重ね着（見た目のみ）を適用する
	ApplyInitialEquipment();

	// AMyProject1Character由来のCameraBoom/FollowCameraはNPCでは使わない。
	// コンストラクタ時点のbVisualizeComponent=falseだけではPIE中の選択時カメラプレビュー（右下の映像）を
	// 防げないため、実行時にコンポーネントごと破棄する
	if (USpringArmComponent* Boom = GetCameraBoom())
	{
		Boom->DestroyComponent();
	}
	if (UCameraComponent* Cam = GetFollowCamera())
	{
		Cam->DestroyComponent();
	}

	if (VisibilityMode == EFlagVisibilityMode::None || VisibilityFlag.IsNone()) return;

	InitFlagVisibilityWhenReady();
}

void AQuestNPCBase::InitFlagVisibilityWhenReady()
{
	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));

	// プレイヤーがまだ生成されていない（レベル遷移直後・ローディング画面中など）。少し待って再試行
	if (!PlayerChar)
	{
		if (FlagVisibilityInitAttempts++ < 240) // 0.25秒 * 240 = 最大60秒粘る
		{
			GetWorldTimerManager().SetTimer(FlagVisibilityInitTimerHandle, this, &AQuestNPCBase::InitFlagVisibilityWhenReady, 0.25f, false);
		}
		return;
	}

	// フラグ通知の購読は一度だけ（同一マップでの「会話→フラグ付与」ライブ経路用）。
	// 別レベルからのUnlockedFlags一括復元はAddFlagを経由しないためOnFlagAddedは飛んでこない → 下のHasFlag直接判定で拾う
	if (!bFlagVisibilityDelegatesBound)
	{
		PlayerChar->OnFlagAdded.AddDynamic(this, &AQuestNPCBase::OnPlayerFlagAdded);
		PlayerChar->OnFlagRemoved.AddDynamic(this, &AQuestNPCBase::OnPlayerFlagRemoved);
		bFlagVisibilityDelegatesBound = true;
		FlagVisibilityInitAttempts = 0; // ここからフラグ復元待ちの試行回数を数え直す
	}

	UpdateFlagVisibility(PlayerChar);

	// プレイヤーは取れたがVisibilityFlagがまだ無い。別レベルからのステータス復元(ApplyPendingCharacterLoad)が
	// このNPCの初期化より後にずれ込むケースを拾うため、数回だけ再チェックしてから諦める（以降はOnFlagAdded/OnFlagRemoved待ち）。
	// ShowOnFlag／HideOnFlagどちらも「後から復元でフラグが現れる」のが危険ケースなので、判定条件は共通で良い
	if (!PlayerChar->HasFlag(VisibilityFlag) && FlagVisibilityInitAttempts++ < 20) // 0.25秒 * 20 = 5秒
	{
		GetWorldTimerManager().SetTimer(FlagVisibilityInitTimerHandle, this, &AQuestNPCBase::InitFlagVisibilityWhenReady, 0.25f, false);
	}
}

void AQuestNPCBase::ApplyInitialEquipment()
{
	if (EquipmentDataTable)
	{
		for (const FName& RowName : InitialEquipmentRowNames)
		{
			if (RowName.IsNone()) continue;

			FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(RowName, TEXT("QuestNPC InitialEquipment"));
			if (!EquipData) continue;

			// EquipItem()は継承元(AMyProject1Character)の実装をそのまま使う。
			// ShouldApplyEquipmentStatBonuses()をfalseに上書きしているため、
			// メッシュ・スキンオーバーレイは反映されるが、MyStatsへのStatModifiers加算は行われない
			EquipItem(RowName, *EquipData);
		}
	}

	// 刺青・傷跡もSkinOverlayComp->AddOverlay()経由でRow名を並べるだけで初期反映する。
	// CustomOpacity=-1.0fはAddOverlay内部でRowのDefaultOpacityを使う指定（装備と同じ「値を渡さない」扱い）
	if (SkinOverlayComp)
	{
		for (const FName& RowName : InitialTattooOverlayRowNames)
		{
			if (RowName.IsNone()) continue;
			SkinOverlayComp->AddOverlay(RowName, -1.0f, EShopModeCategory::Tattoo);
		}

		for (const FName& RowName : InitialScarOverlayRowNames)
		{
			if (RowName.IsNone()) continue;
			SkinOverlayComp->AddOverlay(RowName, -1.0f, EShopModeCategory::Scar);
		}
	}
}

void AQuestNPCBase::UpdateFlagVisibility(const AMyProject1Character* PlayerChar)
{
	const bool bHasFlag = PlayerChar && PlayerChar->HasFlag(VisibilityFlag);
	ApplyVisibilityForFlagState(bHasFlag);
}

void AQuestNPCBase::ApplyVisibilityForFlagState(bool bHasFlag)
{
	const bool bVisible = (VisibilityMode == EFlagVisibilityMode::ShowOnFlag) ? bHasFlag : !bHasFlag;
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);
}

void AQuestNPCBase::OnPlayerFlagAdded(FName FlagName)
{
	if (VisibilityFlag.IsNone() || FlagName != VisibilityFlag) return;
	ApplyVisibilityForFlagState(true);
}

void AQuestNPCBase::OnPlayerFlagRemoved(FName FlagName)
{
	if (VisibilityFlag.IsNone() || FlagName != VisibilityFlag) return;
	ApplyVisibilityForFlagState(false);
}

void AQuestNPCBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 会話前の向き直り中（FacePlayerThenTalk）にNPCが消えると、先行ロックしたプレイヤー入力が
	// 戻らずソフトロックになるため解除する。会話後の戻り（ReturnAfterTalk）は入力ロックしていないので不要
	if (GetWorldTimerManager().IsTimerActive(TurnTimerHandle))
	{
		if (TurnMode == ETurnMode::FacePlayerThenTalk)
		{
			if (IRpgCharacterInterface* Rpg = Cast<IRpgCharacterInterface>(PendingPlayerChar.Get()))
			{
				Rpg->SetInputLocked(false);
			}
		}
		FinishTurn();
	}

	// フラグ表示の初期化待ちタイマーが回っている途中でNPCが消えた場合の後始末
	GetWorldTimerManager().ClearTimer(FlagVisibilityInitTimerHandle);

	Super::EndPlay(EndPlayReason);
}

FRotator AQuestNPCBase::MakeFacePlayerRotation(const AActor* PlayerChar) const
{
	if (!PlayerChar) return GetActorRotation();

	// ヨーのみプレイヤー方向へ（ピッチ/ロールは水平のまま）
	FRotator LookAt = (PlayerChar->GetActorLocation() - GetActorLocation()).Rotation();
	LookAt.Pitch = 0.0f;
	LookAt.Roll = 0.0f;
	return LookAt;
}

void AQuestNPCBase::StartDialogFacingPlayer(AMyProject1Character* PlayerChar, IRpgCharacterInterface* RpgInterface, FName RowName, const FString& LogContext)
{
	if (!PlayerChar || !RpgInterface) return;

	// 回転中の再入。会話へ向けた向き直り中なら二重起動しない。
	// 会話後の戻り回転中なら、それを中断して新しい会話の向き直りを優先する
	if (GetWorldTimerManager().IsTimerActive(TurnTimerHandle))
	{
		if (TurnMode == ETurnMode::FacePlayerThenTalk) return;
		FinishTurn();
	}

	UDialogComponent* PlayerDialogComp = PlayerChar->FindComponentByClass<UDialogComponent>();
	if (!PlayerDialogComp) return;

	const FRotator TargetRot = MakeFacePlayerRotation(PlayerChar);
	const float YawDelta = FMath::Abs(FRotator::NormalizeAxis(TargetRot.Yaw - GetActorRotation().Yaw));

	// 向き直り不要：フラグON、または既にほぼ正対している → 従来通り即座に会話開始
	if (bDoNotTurnToPlayerOnTalk || YawDelta <= 1.0f)
	{
		if (!bDoNotTurnToPlayerOnTalk)
		{
			PreTalkRotation = GetActorRotation();
			bHasPreTalkRotation = true;
		}
		BeginDialogRow(PlayerDialogComp, RpgInterface, RowName, LogContext, /*bDidTurn=*/false);
		return;
	}

	// 向き直ってから会話：先に入力ロック＆向きを退避し、タイマーで一定速度回転させる。
	// 会話は TickFacePlayerTurn の完了時に BeginDialogRow で開始する
	PreTalkRotation = GetActorRotation();
	bHasPreTalkRotation = true;
	RpgInterface->SetInputLocked(true);

	PendingPlayerChar = PlayerChar;
	PendingDialogComp = PlayerDialogComp;
	PendingDialogRow = RowName;
	PendingLogContext = LogContext;

	StartTurnTimer(ETurnMode::FacePlayerThenTalk);
}

void AQuestNPCBase::StartTurnTimer(ETurnMode Mode)
{
	TurnMode = Mode;
	LastTurnTickTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &AQuestNPCBase::TickTurn, 1.0f / 120.0f, true);
}

void AQuestNPCBase::FinishTurn()
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	TurnMode = ETurnMode::None;
}

void AQuestNPCBase::TickTurn()
{
	// 会話へ向けた向き直り中は、対象が消えたら中断して先行ロックした入力と向きを戻す
	if (TurnMode == ETurnMode::FacePlayerThenTalk)
	{
		AMyProject1Character* PlayerChar = PendingPlayerChar.Get();
		UDialogComponent* PlayerDialogComp = PendingDialogComp.Get();
		if (!PlayerChar || !PlayerDialogComp)
		{
			FinishTurn();
			if (IRpgCharacterInterface* Rpg = Cast<IRpgCharacterInterface>(PlayerChar))
			{
				Rpg->SetInputLocked(false);
			}
			if (bHasPreTalkRotation)
			{
				SetActorRotation(PreTalkRotation);
				bHasPreTalkRotation = false;
			}
			return;
		}
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float Step = FMath::Max(Now - LastTurnTickTime, 0.0f);
	LastTurnTickTime = Now;

	// FacePlayerThenTalkはプレイヤーが動く可能性があるので毎tick再計算、ReturnAfterTalkは固定値
	const FRotator TargetRot = (TurnMode == ETurnMode::FacePlayerThenTalk)
		? MakeFacePlayerRotation(PendingPlayerChar.Get())
		: TurnFixedTarget;

	const FRotator NewRot = FMath::RInterpConstantTo(GetActorRotation(), TargetRot, Step, TurnToPlayerSpeedDegPerSec);
	SetActorRotation(NewRot);

	if (!NewRot.Equals(TargetRot, 0.5f)) return;

	// 回転完了
	SetActorRotation(TargetRot);
	const ETurnMode CompletedMode = TurnMode;
	FinishTurn();

	if (CompletedMode == ETurnMode::FacePlayerThenTalk)
	{
		// 向き直り完了 → 会話開始
		BeginDialogRow(PendingDialogComp.Get(), Cast<IRpgCharacterInterface>(PendingPlayerChar.Get()), PendingDialogRow, PendingLogContext, /*bDidTurn=*/true);
	}
	else
	{
		// 会話開始前の向きへ戻し終えた
		bHasPreTalkRotation = false;
	}
}

void AQuestNPCBase::BeginDialogRow(UDialogComponent* PlayerDialogComp, IRpgCharacterInterface* RpgInterface, FName RowName, const FString& LogContext, bool bDidTurn)
{
	if (!PlayerDialogComp || !RpgInterface)
	{
		// 向き直り後にここへ来て開始できない場合、先行ロックを戻す
		if (bDidTurn && RpgInterface)
		{
			RpgInterface->SetInputLocked(false);
		}
		if (bDidTurn && bHasPreTalkRotation)
		{
			SetActorRotation(PreTalkRotation);
			bHasPreTalkRotation = false;
		}
		return;
	}

	if (PlayerDialogComp->TryStartDialog(RowName, DialogTable, this))
	{
		// bDidTurn時は既にロック済みだが冪等なので問題ない
		RpgInterface->SetInputLocked(true);

		// 会話終了通知を購読する（多重購読はAddUniqueDynamicで防止）。解除用にComponentを保持する
		PlayerDialogComp->OnDialogClosed.AddUniqueDynamic(this, &AQuestNPCBase::OnTalkDialogClosed);
		TalkDialogCompBound = PlayerDialogComp;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuestNPC] %s: %s '%s' が未設定/未登録のため会話を開始できませんでした。"),
			*GetName(), *LogContext, *RowName.ToString());

		// 向き直りのために先行してロック・回転していた場合は元に戻す
		if (bDidTurn)
		{
			RpgInterface->SetInputLocked(false);
			if (bHasPreTalkRotation)
			{
				SetActorRotation(PreTalkRotation);
				bHasPreTalkRotation = false;
			}
		}
	}
}

void AQuestNPCBase::OnTalkDialogClosed()
{
	// 購読を解除する（次回の会話でまた購読し直す）
	if (UDialogComponent* BoundComp = TalkDialogCompBound.Get())
	{
		BoundComp->OnDialogClosed.RemoveDynamic(this, &AQuestNPCBase::OnTalkDialogClosed);
	}
	TalkDialogCompBound.Reset();

	if (!bHasPreTalkRotation) return;

	// 会話開始前の向きへ、向き直りと同じ一定速度（TurnToPlayerSpeedDegPerSec）で戻す。
	// ほぼ戻っている場合はタイマーを起こさず即確定する
	const float YawDelta = FMath::Abs(FRotator::NormalizeAxis(PreTalkRotation.Yaw - GetActorRotation().Yaw));
	if (YawDelta <= 1.0f)
	{
		SetActorRotation(PreTalkRotation);
		bHasPreTalkRotation = false;
		return;
	}

	TurnFixedTarget = PreTalkRotation;
	StartTurnTimer(ETurnMode::ReturnAfterTalk);
}

void AQuestNPCBase::TalkToNPC(AMyProject1Character* PlayerChar)
{
	if (!PlayerChar || !DialogTable) return;

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerChar);
	if (!RpgInterface) return;

	// 初回だけの挨拶（フラグ未取得の間だけ表示。Quests配列より優先。
	// フラグの付与は自分では行わず、DT_Dialogs側の対象行のGrantFlagに任せる）
	if (!FirstMeetFlag.IsNone() && !RpgInterface->HasFlag(FirstMeetFlag))
	{
		// 必要ならプレイヤーへ向き直ってから会話開始（行の設定漏れ時はStartDialogFacingPlayer側でロックしない）
		StartDialogFacingPlayer(PlayerChar, RpgInterface, DialogRowName_FirstMeet, TEXT("FirstMeet行"));
		return;
	}

	// 日替わり会話シーケンス（多日クエストの自動進行。FirstMeetの後・ConditionalDialogsの前）。
	// DailyDialogSequenceRows を、ゲーム内で日付が変わるごとに1行ずつ順番に表示する。
	// 進行はプレイヤーのMyStats.DailyDialogProgressに記録され、セーブに残る。
	// ここでは「どの行を表示するか」だけを決め、実際に1歩進めるのはAdvanceDailyDialogSequence()（会話側の
	// bAdvanceDailySequenceがtrueの選択肢／行から呼ばれる）の役目。会話に到達しただけでは進まないので、
	// 「はい/いいえ」のような分岐を挟んで「いいえ」なら進めない、という制御が行側で組める。
	// Wait行／Done行が未設定、または対象行がDialogTableに無い状態では、下のConditionalDialogs判定へフォールスルーする。
	// 今日のステップに条件（MinRank/RequiredStats/RequiredItems）が設定されていて未達の場合も同様にフォールスルーし、
	// 進行（Step/LastAdvanceDay）は更新しない＝条件を満たすまで同じステップのまま待つ
	if (bEnableDailyDialogSequence && !DailyDialogSequenceID.IsNone() && DailyDialogSequenceRows.Num() > 0)
	{
		int32 CurrentDay = 0;
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
		{
			CurrentDay = GameInst->TotalElapsedDays;
		}

		// 参照で受け取ってはいるが、ここでは読むだけ（更新はAdvanceDailyDialogSequence側の責務）
		const FDailyDialogProgress& Progress = RpgInterface->GetCharacterStats().DailyDialogProgress.FindOrAdd(DailyDialogSequenceID);

		FName SeqRowName;
		if (Progress.Step >= DailyDialogSequenceRows.Num())
		{
			// 全行を表示し終えている
			SeqRowName = DailyDialogSequenceDoneRow;
		}
		else if (Progress.LastAdvanceDay >= 0 && CurrentDay <= Progress.LastAdvanceDay)
		{
			// 最後に進めた日から日付が変わっていない＝今日はもう進めた
			SeqRowName = DailyDialogSequenceWaitRow;
		}
		else
		{
			// 日付が変わった（または初回）＝今日のステップの条件を確認してから表示する
			const FDailyDialogSequenceStep& StepData = DailyDialogSequenceRows[Progress.Step];
			const FCharacterStats& PlayerStats = RpgInterface->GetCharacterStats();
			const uint8 PlayerRankValue = static_cast<uint8>(PlayerStats.AdventurerRank);

			bool bStepConditionOk = (StepData.MinRank == EAdventurerRank::None || PlayerRankValue >= static_cast<uint8>(StepData.MinRank));

			if (bStepConditionOk)
			{
				for (const FQuestStatRequirement& StatReq : StepData.RequiredStats)
				{
					if (!StatReq.IsSatisfiedBy(PlayerStats)) { bStepConditionOk = false; break; }
				}
			}

			if (bStepConditionOk && StepData.RequiredItems.Num() > 0)
			{
				UInventoryComponent* PlayerInv = PlayerChar->FindComponentByClass<UInventoryComponent>();
				for (const FDialogItemRequirement& ItemReq : StepData.RequiredItems)
				{
					const int32 Owned = PlayerInv ? PlayerInv->GetItemQuantity(ItemReq.ItemID) : 0;
					if (Owned < ItemReq.RequiredAmount) { bStepConditionOk = false; break; }
				}
			}

			// 条件未達なら何もしない（SeqRowNameはNoneのまま）＝下の判定へフォールスルーし、進行も進めない
			if (bStepConditionOk)
			{
				SeqRowName = StepData.DialogRowName;
			}
		}

		// 対象行が実在する時だけシーケンスとして処理する。未設定／未登録／条件未達なら通常判定へ落とす
		if (!SeqRowName.IsNone() && DialogTable->FindRow<FDialogData>(SeqRowName, TEXT("DailyDialogSequence"), false))
		{
			StartDialogFacingPlayer(PlayerChar, RpgInterface, SeqRowName, TEXT("日替わりシーケンス行"));
			return;
		}
	}

	// 条件付き会話（フラグ／代行者等級で出し分け。FirstMeetの後・Quests配列より優先）。
	// FirstMeetFlagと違い一度きりではなく、話しかけるたびに現在の状態に合った行を出す。
	// どのエントリにも一致しなければreturnせず、そのままQuests配列の判定へ進む
	if (ConditionalDialogs.Num() > 0)
	{
		const FCharacterStats& PlayerStats = RpgInterface->GetCharacterStats();
		const uint8 PlayerRankValue = static_cast<uint8>(PlayerStats.AdventurerRank);
		// アイテム所持数の確認用（無い個体でも落ちないよう、後段でnullガードして0扱いにする）
		UInventoryComponent* PlayerInv = PlayerChar->FindComponentByClass<UInventoryComponent>();

		for (const FConditionalDialogEntry& CondEntry : ConditionalDialogs)
		{
			if (CondEntry.DialogRowName.IsNone()) continue;
			if (!CondEntry.RequiredFlag.IsNone() && !RpgInterface->HasFlag(CondEntry.RequiredFlag)) continue;
			if (!CondEntry.ExcludeFlag.IsNone() && RpgInterface->HasFlag(CondEntry.ExcludeFlag)) continue;
			if (CondEntry.MinRank != EAdventurerRank::None && PlayerRankValue < static_cast<uint8>(CondEntry.MinRank)) continue;

			// 数値ステータス条件（魅力値など）。複数指定は全てAND。1つでも未達ならこのエントリは飛ばす
			bool bStatsOk = true;
			for (const FQuestStatRequirement& StatReq : CondEntry.RequiredStats)
			{
				if (!StatReq.IsSatisfiedBy(PlayerStats)) { bStatsOk = false; break; }
			}
			if (!bStatsOk) continue;

			// 所持アイテム条件。複数指定は全てAND。InventoryComponentが無ければ所持数0として未達扱い
			bool bItemsOk = true;
			for (const FDialogItemRequirement& ItemReq : CondEntry.RequiredItems)
			{
				const int32 Owned = PlayerInv ? PlayerInv->GetItemQuantity(ItemReq.ItemID) : 0;
				if (Owned < ItemReq.RequiredAmount) { bItemsOk = false; break; }
			}
			if (!bItemsOk) continue;

			StartDialogFacingPlayer(PlayerChar, RpgInterface, CondEntry.DialogRowName, TEXT("条件付き会話行"));
			return;
		}
	}

	if (Quests.Num() == 0) return;

	UQuestComponent* PlayerQuestComp = RpgInterface->GetQuestComponent();
	if (!PlayerQuestComp) return;

	TArray<FName> CandidateIDs;
	CandidateIDs.Reserve(Quests.Num());
	for (const FQuestDialogSet& Entry : Quests)
	{
		CandidateIDs.Add(Entry.QuestID);
	}

	// ResolvedQuestIDがNoneになるのは「QuestIDを空欄にした会話専用エントリ」が選ばれた場合もあるため、
	// ここでは弾かない（Quests自体が空の場合は既に上でreturn済み）
	const FName ResolvedQuestID = PlayerQuestComp->GetNextOfferableQuest(CandidateIDs);

	const FQuestDialogSet* Entry = Quests.FindByPredicate([&](const FQuestDialogSet& E) { return E.QuestID == ResolvedQuestID; });
	if (!Entry) return;

	FName RowName;

	// 会話クエストで順番を無視して話しかけられた場合は、進捗は進めず、代わりにNotStarted（世間話・挨拶）を出す
	if (!PlayerQuestComp->IsExpectedTalkTarget(ResolvedQuestID, this))
	{
		RowName = Entry->DialogRowName_NotStarted;
	}
	else
	{
		switch (PlayerQuestComp->GetQuestStatus(ResolvedQuestID))
		{
		case EQuestStatus::NotStarted:
			// 受注条件（フラグ・前提クエスト等）を満たすまでは、クエストの気配を出さないLocked行を表示する。
			// Locked行が未設定のNPCは従来通りNotStarted行をそのまま使う
			if (!PlayerQuestComp->CanAcceptQuest(ResolvedQuestID) && !Entry->DialogRowName_Locked.IsNone())
			{
				RowName = Entry->DialogRowName_Locked;
			}
			else
			{
				RowName = Entry->DialogRowName_NotStarted;
			}
			break;
		case EQuestStatus::InProgress:       RowName = Entry->DialogRowName_InProgress; break;
		case EQuestStatus::ObjectiveCleared: RowName = Entry->DialogRowName_ObjectiveCleared; break;
		case EQuestStatus::Completed:        RowName = Entry->DialogRowName_Completed; break;
		}
	}

	// 会話開始（DialogComponent::CloseDialogがSetInputLocked(false)する処理と対）。
	// 表示行（DialogRowName_InProgress等）が未設定の場合はStartDialogFacingPlayer側でロックしない。
	// 向き直りが必要ならヨー回転が完了してから会話ウィンドウを開く
	StartDialogFacingPlayer(PlayerChar, RpgInterface, RowName,
		FString::Printf(TEXT("クエスト '%s' の表示行"), *ResolvedQuestID.ToString()));
}

void AQuestNPCBase::AdvanceDailyDialogSequence(AMyProject1Character* PlayerChar)
{
	if (!bEnableDailyDialogSequence || DailyDialogSequenceID.IsNone() || !PlayerChar) return;

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerChar);
	if (!RpgInterface) return;

	int32 CurrentDay = 0;
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		CurrentDay = GameInst->TotalElapsedDays;
	}

	FDailyDialogProgress& Progress = RpgInterface->GetCharacterStats().DailyDialogProgress.FindOrAdd(DailyDialogSequenceID);

	// 既に行数の上限に達していても、最後に進めた日だけは更新しておく
	// （Doneの行でも誤ってActionTypeを付けてしまった場合に、Step自体が配列外まで増え続けるのを防ぐ）
	if (Progress.Step < DailyDialogSequenceRows.Num())
	{
		Progress.Step++;
	}
	Progress.LastAdvanceDay = CurrentDay;
}
