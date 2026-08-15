#include "QuestNPCBase.h"
#include "MyProject1Character.h"
#include "SkinOverlayComponent.h"
#include "MyAIController.h"
#include "QuestComponent.h"
#include "DialogComponent.h"
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

	AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerChar) return;

	UpdateFlagVisibility(PlayerChar);
	PlayerChar->OnFlagAdded.AddDynamic(this, &AQuestNPCBase::OnPlayerFlagAdded);
	PlayerChar->OnFlagRemoved.AddDynamic(this, &AQuestNPCBase::OnPlayerFlagRemoved);
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

void AQuestNPCBase::TalkToNPC(AMyProject1Character* PlayerChar)
{
	if (!PlayerChar || !DialogTable) return;

	IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerChar);
	if (!RpgInterface) return;

	// 初回だけの挨拶（フラグ未取得の間だけ表示。Quests配列より優先。
	// フラグの付与は自分では行わず、DT_Dialogs側の対象行のGrantFlagに任せる）
	if (!FirstMeetFlag.IsNone() && !RpgInterface->HasFlag(FirstMeetFlag))
	{
		if (UDialogComponent* PlayerDialogComp = PlayerChar->FindComponentByClass<UDialogComponent>())
		{
			PlayerDialogComp->StartDialog(DialogRowName_FirstMeet, DialogTable, this);
			RpgInterface->SetInputLocked(true);
		}
		return;
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

	if (UDialogComponent* PlayerDialogComp = PlayerChar->FindComponentByClass<UDialogComponent>())
	{
		PlayerDialogComp->StartDialog(RowName, DialogTable, this);

		// 会話中はプレイヤー操作をロックする（DialogComponent::CloseDialogが解除する処理と対）
		RpgInterface->SetInputLocked(true);
	}
}
