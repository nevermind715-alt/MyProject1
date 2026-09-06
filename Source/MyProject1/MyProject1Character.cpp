// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProject1Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "MyProject1.h"
#include "RpgDamageCalculator.h" 
#include "Engine/LocalPlayer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h" 
#include "GameFramework/Pawn.h"     
#include "GameFramework/CharacterMovementComponent.h" 
#include "Kismet/KismetSystemLibrary.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "MyProject1HUD.h"
#include "ShopNPCBase.h"
#include "QuestNPCBase.h"
#include "Blueprint/UserWidget.h"
#include "QuestComponent.h"
#include "DialogComponent.h"
#include "AbilityComponent.h"
#include "MyProject1GameInstance.h"
#include "MusicControlComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PhysicsVolume.h"
#include "CableComponent.h"
#include "SkinOverlayComponent.h"
#include "WallWarpLink.h"
#include "QuestItemPoint.h"
#include "SleepPoint.h"
#include "RestraintBreakPoint.h"



AMyProject1Character::AMyProject1Character()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;


	AbilityComp = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComp"));

	// --- 武器コンポーネントの作成と設定（ここで行う） ---
	WeaponMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComp"));
	// キャラクターの手にアタッチ（ソケット名はスケルトンに合わせて調整）
	WeaponMeshComp->SetupAttachment(GetMesh(), FName("hand_r_socket"));

	// StaticMesh用の武器コンポーネント
	StaticWeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMeshComponent"));
	StaticWeaponMeshComp->SetupAttachment(GetMesh(), FName("hand_r_socket"));
	StaticWeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 武器自体が何かにぶつかってバグるのを防ぐ

	// ==========================================
	// 装備コンポーネントの生成とアタッチ
	// ==========================================

	// 1. 柔らかい装備群（SkeletalMesh）の生成とウェイト連動設定

	HairMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HairMeshComp"));
	HairMeshComp->SetupAttachment(GetMesh());
	HairMeshComp->SetLeaderPoseComponent(GetMesh());

	FaceMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FaceMeshComp"));
	FaceMeshComp->SetupAttachment(GetMesh());
	FaceMeshComp->SetLeaderPoseComponent(GetMesh());

	HeadMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadMeshComp"));
	HeadMeshComp->SetupAttachment(GetMesh());
	HeadMeshComp->SetLeaderPoseComponent(GetMesh());

	TorsoMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TorsoMeshComp"));
	TorsoMeshComp->SetupAttachment(GetMesh());
	TorsoMeshComp->SetLeaderPoseComponent(GetMesh());

	InnerUpperMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("InnerUpperMeshComp"));
	InnerUpperMeshComp->SetupAttachment(GetMesh());
	InnerUpperMeshComp->SetLeaderPoseComponent(GetMesh());

	InnerLowerMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("InnerLowerMeshComp"));
	InnerLowerMeshComp->SetupAttachment(GetMesh());
	InnerLowerMeshComp->SetLeaderPoseComponent(GetMesh());

	WaistMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WaistMeshComp"));
	WaistMeshComp->SetupAttachment(GetMesh());
	WaistMeshComp->SetLeaderPoseComponent(GetMesh());

	HandsMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMeshComp"));
	HandsMeshComp->SetupAttachment(GetMesh());
	HandsMeshComp->SetLeaderPoseComponent(GetMesh());

	LegsMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMeshComp"));
	LegsMeshComp->SetupAttachment(GetMesh());
	LegsMeshComp->SetLeaderPoseComponent(GetMesh());

	FeetMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMeshComp"));
	FeetMeshComp->SetupAttachment(GetMesh());
	FeetMeshComp->SetLeaderPoseComponent(GetMesh());

	WristSkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WristSkeletalMeshComp"));
	WristSkeletalMeshComp->SetupAttachment(GetMesh());
	WristSkeletalMeshComp->SetLeaderPoseComponent(GetMesh());

	NeckSkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NeckSkeletalMeshComp"));
	NeckSkeletalMeshComp->SetupAttachment(GetMesh());
	NeckSkeletalMeshComp->SetLeaderPoseComponent(GetMesh());

	AnkleSkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnkleSkeletalMeshComp"));
	AnkleSkeletalMeshComp->SetupAttachment(GetMesh());
	AnkleSkeletalMeshComp->SetLeaderPoseComponent(GetMesh());

	// 特殊枠（ピアス等、SkeletalMeshで表現するアクセサリー用）
	// ※ピアスのように頂点がボーンの局所範囲にしかない小型メッシュは、自前のバウンズ計算だと
	//   姿勢によっては本来の描画位置とズレて誤カリングされることがあるため、
	//   bUseBoundsFromLeaderPoseComponentで本体メッシュ（毎フレーム正しく更新される）のバウンズをそのまま使う。
	Extra1MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Extra1MeshComp"));
	Extra1MeshComp->SetupAttachment(GetMesh());
	Extra1MeshComp->SetLeaderPoseComponent(GetMesh());
	Extra1MeshComp->bUseBoundsFromLeaderPoseComponent = true;

	Extra2MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Extra2MeshComp"));
	Extra2MeshComp->SetupAttachment(GetMesh());
	Extra2MeshComp->SetLeaderPoseComponent(GetMesh());
	Extra2MeshComp->bUseBoundsFromLeaderPoseComponent = true;

	Extra3MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Extra3MeshComp"));
	Extra3MeshComp->SetupAttachment(GetMesh());
	Extra3MeshComp->SetLeaderPoseComponent(GetMesh());
	Extra3MeshComp->bUseBoundsFromLeaderPoseComponent = true;

	Extra4MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Extra4MeshComp"));
	Extra4MeshComp->SetupAttachment(GetMesh());
	Extra4MeshComp->SetLeaderPoseComponent(GetMesh());
	Extra4MeshComp->bUseBoundsFromLeaderPoseComponent = true;

	Extra5MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Extra5MeshComp"));
	Extra5MeshComp->SetupAttachment(GetMesh());
	Extra5MeshComp->SetLeaderPoseComponent(GetMesh());
	Extra5MeshComp->bUseBoundsFromLeaderPoseComponent = true;


	// 2. 固いアクセサリー群（StaticMesh）の生成とソケットへのアタッチ
	// ※ソケット名("hand_r_socket"等)は後でUE5側でVRMの骨に合わせて調整します
	NeckMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NeckMeshComp"));
	NeckMeshComp->SetupAttachment(GetMesh(), TEXT("Neck_socket"));

	WristMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WristMeshComp"));
	WristMeshComp->SetupAttachment(GetMesh(), TEXT("hand_r_socket"));

	AnkleMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnkleMeshComp"));
	AnkleMeshComp->SetupAttachment(GetMesh(), TEXT("foot_r_socket"));

	// 鎖コンポーネントの生成
	EquipmentCableComp = CreateDefaultSubobject<UCableComponent>(TEXT("EquipmentCableComp"));
	EquipmentCableComp->SetupAttachment(GetMesh());
	EquipmentCableComp->bAttachEnd = true; // 終点を別コンポーネントにアタッチすることを許可
	EquipmentCableComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 鎖自体の衝突判定は基本無効
	EquipmentCableComp->SetVisibility(false);
	EquipmentCableComp->NumSegments = 15;
	// スケール1の空間を維持する中継ダミーの生成
	CableDummyStart = CreateDefaultSubobject<USceneComponent>(TEXT("CableDummyStart"));
	CableDummyStart->SetupAttachment(RootComponent); // スケールが1,1,1のカプセル直下に配置

	CableDummyEnd = CreateDefaultSubobject<USceneComponent>(TEXT("CableDummyEnd"));
	CableDummyEnd->SetupAttachment(RootComponent);   // スケールが1,1,1のカプセル直下に配置

	EquipmentCableComp_Hands = CreateDefaultSubobject<UCableComponent>(TEXT("EquipmentCableComp_Hands"));
	EquipmentCableComp_Hands->SetupAttachment(GetMesh());
	EquipmentCableComp_Hands->bAttachEnd = true;
	EquipmentCableComp_Hands->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquipmentCableComp_Hands->SetVisibility(false);
	EquipmentCableComp_Hands->NumSegments = 15;

	CableDummyStart_Hands = CreateDefaultSubobject<USceneComponent>(TEXT("CableDummyStart_Hands"));
	CableDummyStart_Hands->SetupAttachment(RootComponent);

	CableDummyEnd_Hands = CreateDefaultSubobject<USceneComponent>(TEXT("CableDummyEnd_Hands"));
	CableDummyEnd_Hands->SetupAttachment(RootComponent);

	
	// 武器のサブコンポーネント（鉄球など物理で揺れるパーツ）の生成
	WeaponSubMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponSubMeshComp"));
	WeaponSubMeshComp->SetupAttachment(GetMesh());
	WeaponSubMeshComp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly); // 物理挙動で揺らすため
	WeaponSubMeshComp->SetSimulatePhysics(false); // 初期状態は無効
	WeaponSubMeshComp->SetVisibility(false);

	// ... 既存の CameraBoom や FollowCamera の設定を続ける ...

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 変数の初期化を追記
	bIsPreparingAttack = false;
	StartAttackTimestamp = 0.0;
	AttackStartupDelay = 2.5f;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComp"));
	SkinOverlayComp = CreateDefaultSubobject<USkinOverlayComponent>(TEXT("SkinOverlayComp"));
	QuestComp = CreateDefaultSubobject<UQuestComponent>(TEXT("QuestComp"));
	DialogComp = CreateDefaultSubobject<UDialogComponent>(TEXT("DialogComp"));
	MusicComp = CreateDefaultSubobject<UMusicControlComponent>(TEXT("MusicComp"));

	// スタミナ回復のデフォルト値（後でエディタから変更可能です）
	StaminaRecoveryCombat = 10.0f;
	StaminaRecoveryField = 25.0f;

}

// --- BeginPlay の実装 ---
void AMyProject1Character::BeginPlay()
{
	Super::BeginPlay();

	// ★AnkleAccessoryAnimClassが設定されている場合だけ、足首装備をLeader Pose Componentから切り離し、
	//   専用ABP（Copy Pose from Mesh + ヒール補正の打ち消しノードを想定）に差し替える。
	//   未設定なら何もせず、今まで通りLeader Pose Componentで本体に追従する（＝現状維持）。
	if (AnkleSkeletalMeshComp && AnkleAccessoryAnimClass)
	{
		AnkleSkeletalMeshComp->SetLeaderPoseComponent(nullptr);
		AnkleSkeletalMeshComp->SetAnimInstanceClass(AnkleAccessoryAnimClass);
	}

	ClearCableSystem(EEquipmentSlot::Feet);
	ClearCableSystem(EEquipmentSlot::Hands);

	// --- 疑似飛行の処理：設定した高さだけメッシュを上に持ち上げる ---
	if (HoverHeight > 0.0f)
	{
		if (USkeletalMeshComponent* CharacterMesh = GetMesh())
		{
			FVector CurrentLocation = CharacterMesh->GetRelativeLocation();
			CurrentLocation.Z += HoverHeight;
			CharacterMesh->SetRelativeLocation(CurrentLocation);
		}
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// エディタ側で変数 DefaultMappingContext に IMC_Default などをセットしておく必要があります
			// もし変数がなければ、まずは BP 側で Add Mapping Context ノードを組む形でも動きます
		}
	}

	// セーブロード or 別マップワープでの復元が行われたか。
	// 復元された場合は装備もその内容で再構築されるため、DefaultEquipmentRowNamesは適用しない。
	bool bRestoredFromSnapshot = false;

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		// すぐに呼ぶのではなく、1フレーム待つか、
		// あるいは確実に Controller が設定されていることを確認してから呼ぶ
		// ※IsPlayerControlled()も必須：AIControllerで自動所持されるNPC（AQuestNPCBase等）も
		// GetController()は非nullになるため、これが無いとNPCが「1回だけ消費される」保留中の
		// ワープ座標・セーブ復元を横取りしてしまい、本来のプレイヤーに反映されなくなる
		if (GetController() && IsPlayerControlled())
		{
			GameInst->ApplyPendingWarp(this);
			bRestoredFromSnapshot = GameInst->ApplyPendingCharacterLoad(this);
		}

		// マップも太陽も準備完了したこのタイミングで、初回の時間を通知する ---
		if (GameInst->OnInGameTimeChanged.IsBound())
		{
			int32 Hour = GameInst->CurrentTimeInMinutes / 60;
			int32 Minute = GameInst->CurrentTimeInMinutes % 60;
			GameInst->OnInGameTimeChanged.Broadcast(GameInst->CurrentYear, GameInst->CurrentMonth, GameInst->CurrentDay, Hour, Minute);
		}

		if (!GameInst->OnDayChangedDelegate.IsAlreadyBound(this, &AMyProject1Character::UpdateCycleState))
		{
			GameInst->OnDayChangedDelegate.AddDynamic(this, &AMyProject1Character::UpdateCycleState);
		}

	}

	// ゲーム開始時にデータテーブルの情報をキャラクターに反映させる
	ApplyJobData();

	// 完全新規開始（セーブロードでも別マップワープでもない初回Play）のときだけ初期装備を着せる。
	// ApplyJobData()が素体メッシュ・ジョブ既定の髪をセットした後に呼ぶこと（順序が重要）。
	if (IsPlayerControlled() && !bRestoredFromSnapshot)
	{
		ApplyDefaultEquipment();
	}

	UpdateHealthWidgetName(CharacterName);

	LastCombatTime = GetWorld()->GetTimeSeconds();

	if (AutoRecoveryInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_AutoRecovery, this, &AMyProject1Character::HandleAutoRecovery, AutoRecoveryInterval, true);
	}

	// 疲労度の計算を「1秒に1回」のペースで自動実行する
	GetWorldTimerManager().SetTimer(TimerHandle_FatigueUpdate, this, &AMyProject1Character::HandleFatigueTick, 1.0f, true);
	// --- 最初のまばたきまでの時間をセット ---
	TimeUntilNextBlink = FMath::RandRange(BlinkIntervalMin, BlinkIntervalMax);
}

void AMyProject1Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent); // 親のバインドを維持
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		
		// Jumping
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		//EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProject1Character::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMyProject1Character::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyProject1Character::Look);

		// ホイール入力のバインド
		if (ZoomAction)
		{
			EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AMyProject1Character::ZoomCamera);
		}

		

		// Space(JumpAction)：ターゲット中のNPC/敵に応じて会話開始・ショップ開始・戦闘開始を行う
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyProject1Character::OnActionKeyPressed);
		}

		// Tab(ToggleMenuAction)：MaineWindow(コマンドメニュー)の開閉
		if (ToggleMenuAction)
		{
			EnhancedInputComponent->BindAction(ToggleMenuAction, ETriggerEvent::Started, this, &AMyProject1Character::OnToggleMenuPressed);
		}

		// G/H等(WaitAction)：その場で時間を進める「待機」メニューを開く
		if (WaitAction)
		{
			EnhancedInputComponent->BindAction(WaitAction, ETriggerEvent::Started, this, &AMyProject1Character::OnWaitKeyPressed);
		}

	}


}

void AMyProject1Character::OnActionKeyPressed()
{
	// 会話中・カットシーン中などの操作ロック中は何もしない
	if (bIsInputLocked) return;

	if (!CurrentTarget) return;

	// 会話クエストNPCなら会話開始（コマンドメニューが開いていたら先に閉じる排他処理）
	if (AQuestNPCBase* QuestNPC = Cast<AQuestNPCBase>(CurrentTarget))
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
			{
				HUD->ForceCloseCommandMenuForInteract();
			}
		}

		QuestNPC->TalkToNPC(this);
		return;
	}

	// ショップNPCならショップ開始（同上、排他処理）
	if (AShopNPCBase* ShopNPC = Cast<AShopNPCBase>(CurrentTarget))
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
			{
				HUD->ForceCloseCommandMenuForInteract();
			}
		}

		ShopNPC->OpenShop(this);
		return;
	}

	// アイテム授受ギミック（お供え物・ゴミ拾い等）ならアイテムのやり取りを実行
	if (AQuestItemPoint* ItemPoint = Cast<AQuestItemPoint>(CurrentTarget))
	{
		ItemPoint->TryInteract(this);
		return;
	}

	// ベッド/布団の睡眠ポイントなら睡眠メニューを開く
	if (ASleepPoint* SleepPoint = Cast<ASleepPoint>(CurrentTarget))
	{
		SleepPoint->TryInteract(this);
		return;
	}

	// 拘束具の破壊ポイントなら破壊解除を試みる
	if (ARestraintBreakPoint* BreakPoint = Cast<ARestraintBreakPoint>(CurrentTarget))
	{
		BreakPoint->TryInteract(this);
		return;
	}

	// Enemyタグなら攻撃開始/解除をトグル
	if (CurrentTarget->ActorHasTag(FName("Enemy")))
	{
		ToggleCombatMode();
		return;
	}

	// QuestNPC/ShopNPC/Enemyのどれでもない場合（クエストボード等のBlueprint専用Interactable）は
	// BP側の実装（Eキーと同じInteract呼び出し）に委ねる。
	// Eキー側はInteract前にコマンドメニューを閉じる排他処理を行っているので、Spaceキーでも同様に行う
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
		{
			HUD->ForceCloseCommandMenuForInteract();
		}
	}

	BP_TryInteractWithTarget(CurrentTarget);
}

void AMyProject1Character::OnWaitKeyPressed()
{
	TryOpenTimeSkipMenu(false);
}

bool AMyProject1Character::TryOpenTimeSkipMenu(bool bIsSleepMode)
{
	// 会話中・カットシーン中・戦闘中（オートアタック中）は時間を進めさせない
	if (bIsInputLocked || bIsInCutscene || bIsAutoAttacking)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	AMyProject1HUD* HUD = PC ? Cast<AMyProject1HUD>(PC->GetHUD()) : nullptr;
	if (!HUD)
	{
		return false;
	}

	HUD->OpenTimeSkipMenu(bIsSleepMode);
	return true;
}

void AMyProject1Character::OnToggleMenuPressed()
{
	// Tabキー入力があったのでログウィンドウを再表示させる
	OnRequestShowLogWindow();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD());
		if (HUD)
		{
			bool bIsCommandMenuOpen = (HUD->CommandMenuWidget && HUD->CommandMenuWidget->IsInViewport());

			// 「メニューは閉じていて」かつ「操作ロック中（＝ショップや会話中）」なら、メニューを開かせない
			if (!bIsCommandMenuOpen && bIsInputLocked)
			{
				return;
			}

			// 「メニューが開いていて」かつ「戦闘中」なら、誤ってメニューを消せないようにする
			if (bIsCommandMenuOpen && (bIsAutoAttacking || bIsPreparingAttack))
			{
				return;
			}

			HUD->ToggleCommandMenu();
		}
	}
}

// 2. 関数の実装
void AMyProject1Character::ToggleCombatMode()
{
	if (bIsAutoAttacking || bIsPreparingAttack)
	{
		SetCurrentTarget(nullptr);
		bIsAutoAttacking = false;
		bIsPreparingAttack = false;
		OnReceiveLogMessage(TEXT("戦闘解除"), ELogMessageType::System);

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
			{
				if (HUD->CommandMenuWidget && HUD->CommandMenuWidget->IsInViewport())
				{
					HUD->ToggleCommandMenu();
				}
			}
		}
	}
	else if (CurrentTarget)
	{
		// 直接攻撃せず、StartAutoAttack(準備)を呼ぶ
		StartAutoAttack();

		// 戦闘開始と同時にMaineMenuを表示する
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
			{
				if (!HUD->IsCommandMenuOpen())
				{
					HUD->ToggleCommandMenu();
				}
			}
		}
	}
}

void AMyProject1Character::SetInputLocked(bool bLocked)
{
	bIsInputLocked = bLocked;

	if (bLocked)
	{
		// ロックした瞬間に移動を即座に停止させる（慣性で滑るのを防ぐ）
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}
	}
}

void AMyProject1Character::HandleJumpCompleted()
{
	// BP側のイベントを呼び出す
	OnSpaceActionReleased();
}

void AMyProject1Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMyProject1Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMyProject1Character::DoMove(float Right, float Forward)
{
	if (bIsInputLocked) return; // ロック中なら何もしない

	// WASD入力があったのでログウィンドウを再表示させる
	OnRequestShowLogWindow();


	
	if (GetController() != nullptr)
	{
		FRotator Rotation;

		// ★修正ポイント：戦闘モードなら「自分の向き」を、通常なら「カメラの向き」を使う
		if (bIsAutoAttacking && CurrentTarget)
		{
			// 敵を向いている「自分の向き」を基準にする
			Rotation = GetActorRotation();
		}
		else
		{
			// 自由に走り回る「カメラの向き」を基準にする
			Rotation = GetController()->GetControlRotation();
		}

		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 前後方向のベクトル
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// 左右方向のベクトル 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 入力を適用
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMyProject1Character::DoLook(float Yaw, float Pitch)
{
	//if (bIsInputLocked) return; // ロック中なら何もしない

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		// メニュー表示中（bShowMouseCursor が true）のとき
		if (PC->bShowMouseCursor)
		{
			// 右クリック（RightMouseButton）が押されていなければ、回転処理を飛ばす
			if (!PC->IsInputKeyDown(EKeys::RightMouseButton))
			{
				return;
			}
		}

		// 回転を適用
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMyProject1Character::DoJumpStart()
{
	if (bIsInputLocked) return; // ロック中なら何もしない

	// signal the character to jump
	Jump();
}

void AMyProject1Character::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AMyProject1Character::TargetNearestEnemy()
{
	// 1. 周囲のすべての「Pawn（キャラクター）」を探す
	TArray<AActor*> FoundActors;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

	AActor* ClosestActor = nullptr;
	float MinDistance = TargetingRange; // 最初は最大射程をセット

	// 2. 見つかった全員をチェック
	for (AActor* Actor : FoundActors)
	{
		// 自分自身はターゲットしない
		if (Actor == this || !Actor) continue;

		// 非表示中のアクター（AQuestItemPointのRequiredFlag未達時など）はタグを持っていても対象にしない
		if (Actor->IsHidden()) continue;

		// キャストする前に、Actor自体が対象のタグを持っているかチェックする！
		if (!Actor->ActorHasTag(FName("Enemy")) && !Actor->ActorHasTag(FName("NPC"))) continue;

		// 壁抜けワープ（AWallWarpLink）は、TargetingRangeの固定半径ではなく、
		// 実際にボックスへ触れている間だけターゲット対象にする
		if (AWallWarpLink* WarpLink = Cast<AWallWarpLink>(Actor))
		{
			if (!WarpLink->IsReadyToInteract()) continue;
		}

		// 戦闘キャラ(MyProject1Character)の場合のみ、死んでいるかをチェックする
		AMyProject1Character* TargetChar = Cast<AMyProject1Character>(Actor);
		if (TargetChar && TargetChar->IsDead()) continue;

		// 距離を測る
		float Dist = GetDistanceTo(Actor);

		// NPCは自動解除の判定と同じInteractRangeを射程にする（TargetingRangeで拾うと、
		// InteractRangeより遠いNPCを一瞬ターゲットした直後に自動解除ログが出てしまうため）
		const bool bIsNPCActor = Actor->ActorHasTag(FName("NPC"));
		const float AcquireRange = bIsNPCActor ? InteractRange : TargetingRange;
		if (Dist > AcquireRange) continue;

		// 「今の最小距離」より近ければ、候補を更新
		if (Dist < MinDistance)
		{
			MinDistance = Dist;
			ClosestActor = Actor;
		}
	}

	// 3. 一番近かった敵を CurrentTarget にセット（見つからなければ nullptr になる）
	SetCurrentTarget(ClosestActor);

	// ログに出して確認（あとで消してOK）
	if (CurrentTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Target Locked: %s"), *CurrentTarget->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Target Found"));
	}
}

// ターゲット解除
void AMyProject1Character::CancelTarget()
{
	// これを呼ぶことで、OnTargetUpdated(..., false) が発火し、HPバーが消えます
	SetCurrentTarget(nullptr);
}

void AMyProject1Character::CycleTarget()
{
	// 1. 周囲のアクターを全員探す
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

	// 2. 「射程内」のアクターだけのリストを作る
	TArray<AActor*> ValidTargets;

	// 現在自分が戦闘モード（抜刀または準備中）かどうかを判定
	bool bIsInCombatMode = (bIsAutoAttacking || bIsPreparingAttack);

	for (AActor* Actor : FoundActors)
	{
		if (Actor == this || !Actor) continue;

		// 非表示中のアクター（AQuestItemPointのRequiredFlag未達時など）はタグを持っていても対象にしない
		if (Actor->IsHidden()) continue;

		// 自分が戦闘モードの時は、NPCを除外し、Enemyだけを対象にする
		if (bIsInCombatMode)
		{
			if (Actor->ActorHasTag(FName("NPC"))) continue;
			if (!Actor->ActorHasTag(FName("Enemy"))) continue;
		}
		else
		{
			// 非戦闘時は、EnemyとNPCの両方を対象にする
			if (!Actor->ActorHasTag(FName("Enemy")) && !Actor->ActorHasTag(FName("NPC"))) continue;
		}

		// 壁抜けワープ（AWallWarpLink）は、TargetingRangeの固定半径ではなく、
		// 実際にボックスへ触れている間だけターゲット対象にする
		if (AWallWarpLink* WarpLink = Cast<AWallWarpLink>(Actor))
		{
			if (!WarpLink->IsReadyToInteract()) continue;
		}

		// 戦闘キャラ(MyProject1Character)の場合のみ、死んでいるかをチェックする
		AMyProject1Character* TargetChar = Cast<AMyProject1Character>(Actor);
		if (TargetChar && TargetChar->IsDead()) continue;

		// NPCは自動解除の判定と同じInteractRangeを射程にする（TargetNearestEnemy()と揃える）
		const bool bIsNPCActor = Actor->ActorHasTag(FName("NPC"));
		const float AcquireRange = bIsNPCActor ? InteractRange : TargetingRange;

		if (GetDistanceTo(Actor) <= AcquireRange) // 射程内か？
		{
			ValidTargets.Add(Actor);
		}
	}

	// 候補がいなければ終了
	if (ValidTargets.Num() == 0) return;

	// 3. もし今のターゲットがいなければ、一番近いアクターを選ぶ
	if (CurrentTarget == nullptr)
	{
		// ※ TargetNearestEnemy() も内部でEnemyとNPC両方を拾う設定なのでそのまま使えます
		TargetNearestEnemy();
		return;
	}

	// 4. 今のターゲットがリストの「何番目」にいるか探す
	int32 CurrentIndex = ValidTargets.Find(CurrentTarget);

	// 5. 次の番号を計算する（最後の番号なら0番に戻る）
	int32 NextIndex;
	if (CurrentIndex == INDEX_NONE)
	{
		// リストに見つからなかったら0番目（最初の候補）にする
		NextIndex = 0;
	}
	else
	{
		// (今の番号 + 1) を 全体の人数 で割った余りを使うと、自動でループする
		NextIndex = (CurrentIndex + 1) % ValidTargets.Num();
	}

	// 6. 新しいターゲットをセット
	SetCurrentTarget(ValidTargets[NextIndex]);
}

void AMyProject1Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	// 1. 既存の鎖システム（足・その他用）のワールド同期
	if (EquipmentCableComp && EquipmentCableComp->IsVisible())
	{
		if (CurrentCableSourceComponent && !CurrentCableSourceSocket.IsNone() && CableDummyStart)
		{
			FVector SourceLocation = CurrentCableSourceComponent->GetSocketLocation(CurrentCableSourceSocket);
			CableDummyStart->SetWorldLocation(SourceLocation);
		}
		if (CurrentCableTargetComponent && !CurrentCableTargetSocket.IsNone() && CableDummyEnd)
		{
			FVector TargetLocation = CurrentCableTargetComponent->GetSocketLocation(CurrentCableTargetSocket);
			CableDummyEnd->SetWorldLocation(TargetLocation);
		}
	}

	// === 手・腕用の鎖システムのワールド同期 ===
	if (EquipmentCableComp_Hands && EquipmentCableComp_Hands->IsVisible())
	{
		if (CurrentCableSourceComponent_Hands && !CurrentCableSourceSocket_Hands.IsNone() && CableDummyStart_Hands)
		{
			FVector SourceLocation = CurrentCableSourceComponent_Hands->GetSocketLocation(CurrentCableSourceSocket_Hands);
			CableDummyStart_Hands->SetWorldLocation(SourceLocation);
		}
		if (CurrentCableTargetComponent_Hands && !CurrentCableTargetSocket_Hands.IsNone() && CableDummyEnd_Hands)
		{
			FVector TargetLocation = CurrentCableTargetComponent_Hands->GetSocketLocation(CurrentCableTargetSocket_Hands);
			CableDummyEnd_Hands->SetWorldLocation(TargetLocation);
		}
	}

	// --- 周囲の敵のHPバー表示ロジック ---
	if (!IsPlayerControlled() && !IsDead()) // 自分がNPCで、かつ生きている場合
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (PlayerPawn)
		{
			float DistanceToPlayer = GetDistanceTo(PlayerPawn);
			AMyProject1Character* PlayerChar = Cast<AMyProject1Character>(PlayerPawn);

			// 条件：プレイヤーとの距離が範囲内、もしくは現在プレイヤーにターゲットされている
			bool bShouldShow = (DistanceToPlayer <= HPBarDisplayRange) || (PlayerChar && PlayerChar->CurrentTarget == this);

			// 状態が変わった時だけイベントを呼ぶ（最適化）
			if (bShouldShow != bLastHPBarVisibility)
			{
				bLastHPBarVisibility = bShouldShow;
				OnHPBarVisibilityChanged(bShouldShow);
			}
		}
	}

	// --- 1. 死んでいる場合は目を閉じて終了、生きている場合はまばたきを更新 ---
	if (IsDead())
	{
		if (GetMesh())
		{
			// モーフターゲットの値を 1.0（完全に閉じた状態）に固定
			GetMesh()->SetMorphTarget(BlinkMorphName, 1.0f);
		}
		return;
	}
	else
	{
		// 通常時の自動まばたき処理
		UpdateBlink(DeltaTime);
	}

	if (MyStats.Stamina < MyStats.MaxStamina)
	{
		// 戦闘中（ターゲットあり）か、非戦闘中かで回復スピードを切り替える
		float CurrentRecoveryRate = (CurrentTarget != nullptr) ? StaminaRecoveryCombat : StaminaRecoveryField;

		// 毎フレームの経過時間（DeltaTime）を掛けて滑らかに回復
		MyStats.Stamina = FMath::Min(MyStats.Stamina + (CurrentRecoveryRate * DeltaTime), MyStats.MaxStamina);

		// 先ほど作ったスタミナ用のデリゲート（合図）を毎フレーム飛ばしてUIをリアルタイム更新
		if (OnStaminaChangedDelegate.IsBound())
		{
			OnStaminaChangedDelegate.Broadcast(MyStats.Stamina, MyStats.MaxStamina);
		}
	}

	// --- 2. 基本となる目標速度（TargetSpeed）を計算する ---
	float TargetSpeed = LandWalkSpeed; // 基本は陸上の歩行速度

	if (CurrentTarget != nullptr)
	{
		// 戦闘中（ターゲットあり）なら追いかけ速度
		TargetSpeed = ChaseRunSpeed;
	}
	else
	{
		// 非戦闘中なら、プレイヤーは通常の歩行、NPCならパトロール速度
		TargetSpeed = IsPlayerControlled() ? LandWalkSpeed : PatrolWalkSpeed;
	}

	// --- 3. 水の中にいる判定：目標速度を水中用に上書きする ---
	if (GetCharacterMovement() && GetCharacterMovement()->GetPhysicsVolume())
	{
		if (GetCharacterMovement()->GetPhysicsVolume()->bWaterVolume)
		{
			TargetSpeed = WaterWalkSpeed; // 水中なら速度を上書き
		}
	}

	// --- 3.5 拘束具（足枷等）装備中は、状況に関わらず速度に上限をかける ---
	if (bIsMovementRestricted && RestrainedSpeedCap > 0.0f)
	{
		TargetSpeed = FMath::Min(TargetSpeed, RestrainedSpeedCap);
	}

	// --- 4. 最終的な速度を一括で適用する ---
	if (GetCharacterMovement()->MaxWalkSpeed != TargetSpeed)
	{
		GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
	}

	//ターゲットから離れすぎたら自動解除する
	if (CurrentTarget != nullptr && IsPlayerControlled())
	{
		// 壁抜けワープ（AWallWarpLink）は、距離ではなくボックスから出たかどうかで即座に解除する
		if (AWallWarpLink* WarpLink = Cast<AWallWarpLink>(CurrentTarget))
		{
			if (!WarpLink->IsReadyToInteract())
			{
				CancelTarget();
			}
		}
		else
		{
			float DistanceToTarget = GetDistanceTo(CurrentTarget);

			// NPC/宝箱などの近距離インタラクト対象は、InteractRange基準の短い距離で解除する。
			// 敵（戦闘ロックオン）は従来通りTargetingRange基準のまま（境界でのカーソル点滅防止バッファも維持）。
			const bool bIsInteractTarget = CurrentTarget->ActorHasTag(FName("NPC"));
			const float CancelDistance = bIsInteractTarget ? (InteractRange + 100.0f) : (TargetingRange + 500.0f);

			if (DistanceToTarget > CancelDistance)
			{
				CancelTarget(); // これを呼ぶだけで、UIのカーソルが消えます
				OnReceiveLogMessage(TEXT("ターゲットから離れすぎたため解除しました。"), ELogMessageType::System);
			}
		}
	}

	// --- 5. ターゲットがいない場合のリセット処理と早期終了 ---
	if (CurrentTarget == nullptr)
	{
		if (GetCharacterMovement() && !GetCharacterMovement()->bOrientRotationToMovement)
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}
		bUseControllerRotationYaw = false;
		bIsPreparingAttack = false;
		bIsAutoAttacking = false;

		// ターゲットが外れた（＝納刀状態）ならフィールド曲に戻す
		if (IsPlayerControlled() && MusicComp && !IsDead())
		{
			MusicComp->SetCombatMusicActive(false);
		}
		return; // ★重要：ターゲットがいない時はここで処理を終わらせる（速度設定は上で終わっているので安全！）
	}

	// --- 6. エリア離脱（ナビメッシュ外）の判定 ---
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys && CurrentTarget && !CurrentTarget->ActorHasTag(FName("NPC")))
	{
		FNavLocation NavLoc;
		// 戦闘中のターゲット喪失を防ぐため、判定範囲を拡大
		bool bIsOnNavMesh = NavSys->ProjectPointToNavigation(CurrentTarget->GetActorLocation(), NavLoc, FVector(500.f, 500.f, 1000.f));

		if (!bIsOnNavMesh)
		{
			SetCurrentTarget(nullptr);
			bIsAutoAttacking = false;
			bIsPreparingAttack = false;
			return;
		}
	}

	// --- 7. 回転制御（ターゲットの方向を向く） ---
	bool bIsNPC = !IsPlayerControlled();
	bool bShouldFaceTarget = bIsAutoAttacking || bIsPreparingAttack || (bIsNPC && CurrentTarget);

	if (bShouldFaceTarget)
	{
		FVector StartLocation = GetActorLocation();
		FVector TargetLocation = CurrentTarget->GetActorLocation();
		FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);

		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		bUseControllerRotationYaw = false;
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}

		FRotator CurrentActorRot = GetActorRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentActorRot, TargetRotation, DeltaTime, 15.0f);
		SetActorRotation(NewRot);
	}
	else
	{
		if (GetCharacterMovement() && !GetCharacterMovement()->bOrientRotationToMovement)
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}
	}

	// --- 8. 攻撃タイマーの更新ロジック ---
	double CurrentTime = GetWorld()->GetTimeSeconds();
	float DistanceToTarget = GetDistanceTo(CurrentTarget);

	if (bIsPreparingAttack)
	{
		if (CurrentTime - StartAttackTimestamp >= AttackStartupDelay)
		{
			bIsPreparingAttack = false;
			bIsAutoAttacking = true;
		}
	}

	if (bIsAutoAttacking)
	{
		if (!bIsUsingSpecialAttack)
		{
			if (CurrentTime - LastAttackTime >= GetModifiedAttackSpeed())
			{
				if (DistanceToTarget <= AttackRange)
				{
					if (!TryUseSpecialAttack())
					{
						// 特殊技の条件を満たしていなかった場合のみ、通常攻撃を行う
						PerformAutoAttack();

						// 通常攻撃を行ったらカウントを+1する
						ConsecutiveAttackCount++;
					}
					LastAttackTime = CurrentTime;
				}
				else
				{
					if (IsPlayerControlled())
					{
						OnReceiveLogMessage(TEXT("ターゲットが遠すぎます。"), ELogMessageType::Default);
					}
					LastAttackTime = CurrentTime;
				}
			}
		}
	}
}

float AMyProject1Character::UpdateHealth(float Amount)
{
	if (bIsDead) return 0.0f;

	float OldHP = MyStats.HP; // 更新前の値を保持
	MyStats.HP = FMath::Clamp(MyStats.HP + Amount, 0.0f, MyStats.MaxHP);

	// 実際に変動した差分を計算
	float ActualDelta = MyStats.HP - OldHP;

	if (OnHPChangedDelegate.IsBound())
	{
		OnHPChangedDelegate.Broadcast(MyStats.HP, MyStats.MaxHP);
	}
	OnHPChanged(MyStats.HP, MyStats.MaxHP);

	return ActualDelta; // 実際に変化した量を返す
}


float AMyProject1Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	LastCombatTime = GetWorld()->GetTimeSeconds();

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (bIsDead) return 0.0f;

	if (ActualDamage >= 1.0f)
	{
		UpdateHealth(-ActualDamage);

		// ダメージタイプが「UCriticalDamageType」かどうかをチェックする条件を追加
		if (HitReactMontage && DamageEvent.DamageTypeClass == UCriticalDamageType::StaticClass())
		{
			UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
			if (AnimInst)
			{
				// 攻撃中（モンタージュ再生中）でなければのけぞる
				if (!AnimInst->IsAnyMontagePlaying())
				{
					PlayAnimMontage(HitReactMontage);
				}
			}
		}
	}

	

	
	// 2. リンク・オートターゲット処理（既存のロジックをそのまま維持）
	if (CurrentTarget == nullptr && EventInstigator && EventInstigator->GetPawn())
	{
		AActor* Attacker = EventInstigator->GetPawn();
		// プレイヤー以外（他の敵など）からのダメージでは反撃ターゲット化しない
		if (Attacker != this && Attacker->ActorHasTag(FName("Player")))
		{
			SetCurrentTarget(Attacker);
			// 周囲の仲間にリンクを通知（これが敵のリンクの核心です）
			NotifyNearbyAllies(Attacker);
		}
	}

	
	// 3. 死亡判定
	if (MyStats.HP <= 0.0f)
	{
		if (EventInstigator && EventInstigator->GetPawn())
		{
			AMyProject1Character* Killer = Cast<AMyProject1Character>(EventInstigator->GetPawn());
			if (Killer)
			{
				// --- 経験値の動的計算 ---
				int32 BaseExp = this->MyStats.ExperienceReward; // データテーブル等の基本値 (例: 100)
				int32 LevelDiff = this->MyStats.Level - Killer->MyStats.Level; // 敵Lv - 自分Lv
				int32 FinalExp = 0;

				if (LevelDiff <= -5)
				{
					// 弱すぎる敵：経験値5
					FinalExp = 5;
				}
				else if (LevelDiff < 0)
				{
					// 格下：レベル差に応じて減衰 (80% ～ 10%)
					float Penalty[] = { 0.8f, 0.6f, 0.3f, 0.1f }; // -1, -2, -3, -4
					FinalExp = BaseExp * Penalty[FMath::Abs(LevelDiff) - 1];
				}
				else if (LevelDiff == 0)
				{
					// 丁度よい相手：100%
					FinalExp = BaseExp;
				}
				else if (LevelDiff <= 3)
				{
					// 【Tier 1: +1～3】 安全に稼げる (150 ～ 250)
					FinalExp = BaseExp + (LevelDiff * 50);
				}
				else if (LevelDiff <= 6)
				{
					// 【Tier 2: +4～6】 苦戦するが美味しい (300 ～ 400)
					// 300(Lv+4), 350(Lv+5), 400(Lv+6)
					FinalExp = BaseExp + 100 + ((LevelDiff - 3) * 50);
				}
				else
				{
					// 【Tier 3: +7以上】 ほぼ倒せない相手へのジャックポット
					FinalExp = BaseExp * 5; // 基本値の5倍 (例: 500)
				}

				// ログの表示
				FString DefeatMsg = FString::Printf(TEXT("%sを倒した！"), *MyStats.NPCName);
				Killer->OnReceiveLogMessage(DefeatMsg, ELogMessageType::System);

				// 計算した経験値を加算
				Killer->AddExperience(FinalExp);

				for (const FLootItem& Loot : this->PersonalLootTable)
				{
					// 0.0〜100.0のサイコロを振り、確率以下ならアタリ！
					if (FMath::RandRange(0.0f, 100.0f) <= Loot.DropRate)
					{
						if (Killer->InventoryComp)
						{
							Killer->InventoryComp->AddItem(Loot.ItemID, Loot.Quantity);

						}
					}
				}

				if (Killer->QuestComp)
				{
					// 倒した敵の「ジョブの行名（Goblinなど）」をターゲットIDとしてクエストに通知
					Killer->QuestComp->UpdateKillObjective(FName(*this->MyStats.NPCName));
				}

				OnDeath();

				// プレイヤー（Killer）に対して、ターゲットが死んだことを通知する
	            // これによりプレイヤーは攻撃を止め、次の敵がいれば自動でターゲットします
				Killer->HandleTargetDeath();
			}
		}
		
	}

	return ActualDamage;
}

#if WITH_EDITOR
void AMyProject1Character::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// エディタ上で JobRow または Level を変更した際に自動更新
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMyProject1Character, JobRow) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(FCharacterStats, Level)) // MyStats内のLevel
	{
		ApplyJobData();
	}
}
#endif

void AMyProject1Character::StopAutoAttack()
{
	// もしタイマーハンドルなどを使って攻撃ループさせているなら、ここでClearTimerする
	// 今回の仕組みでは Tick で管理しているので、単純にターゲットを外せば攻撃は止まります
	SetCurrentTarget(nullptr);
}

void AMyProject1Character::OnDeath()
{
	if (bIsDead) return;
	bIsDead = true; // 最初にフラグを立てる

	GetWorldTimerManager().ClearAllTimersForObject(this);

	// 1. ターゲット関係を即座に消去
	SetCurrentTarget(nullptr);
	bIsAutoAttacking = false;
	bIsPreparingAttack = false;

	// 1秒かけてメッシュを地面の高さに滑らかに落とす！
	if (HoverHeight > 0.0f)
	{
		FVector TargetLoc = GetMesh()->GetRelativeLocation();
		TargetLoc.Z -= HoverHeight; // 浮かせていた分を引いた「本来の地面の高さ」

		// 実行用のダミー情報（今回は終わった後の通知は不要なのでこれでOK）
		FLatentActionInfo LatentInfo;
		LatentInfo.CallbackTarget = this;

		// メッシュを TargetLoc の位置まで、1.0f 秒かけて移動させる
		UKismetSystemLibrary::MoveComponentTo(
			GetMesh(),
			TargetLoc,
			GetMesh()->GetRelativeRotation(),
			false, // イーズアウト（ゆっくり止まるか）
			false, // イーズイン（ゆっくり動き出すか）
			1.0f,  // ここで「1秒かけて落とす」を直接指定しています！
			false,
			EMoveComponentAction::Move,
			LatentInfo
		);
	}

	else // --- 【ここから追加】地上にいるキャラ（VRMなど）の浮きを補正する ---
	{
		FVector CurrentLoc = GetMesh()->GetRelativeLocation();
		// 浮き具合に合わせて数値を調整してください（-15.0fなど）
		CurrentLoc.Z -= 5.0f;
		GetMesh()->SetRelativeLocation(CurrentLoc);
	}

	// 2. 物理挙動とコリジョン停止（BPエラー防止のため真っ先に止める）
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// 3. ラグドール化と削除予約
	//GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	//GetMesh()->SetSimulatePhysics(true);

	if (IsPlayerControlled() && MusicComp)
	{
		MusicComp->PlayDeathMusic();
	}

	if (bDestroyOnDeath)
	{
		SetLifeSpan(DeathLifeSpan);
	}
	
	if (OnDeathDelegate.IsBound())
	{
		OnDeathDelegate.Broadcast(this);
	}
}


void AMyProject1Character::PerformAutoAttack()
{
	// 特殊技のモーション中は、呼び出し元（Tick／BTタスク問わず）に関係なく
	// 通常攻撃をここで確実にブロックする（呼び出し元だけのチェックだと抜け道が残るため）
	if (bIsUsingSpecialAttack) return;

	LastCombatTime = GetWorld()->GetTimeSeconds();

	// ジョブデータを取得
	FJobAttributes* JobData = JobRow.GetRow<FJobAttributes>(JobRow.RowName.ToString());

	// データがあり、かつモンタージュが1つ以上登録されているかチェック
	if (JobData && JobData->AttackMontages.Num() > 0)
	{
		// ランダムにインデックスを選択 (0 ～ 配列の最後)
		int32 RandomIndex = FMath::RandRange(0, JobData->AttackMontages.Num() - 1);
		UAnimMontage* SelectedMontage = JobData->AttackMontages[RandomIndex];

		if (SelectedMontage)
		{
			PlayAnimMontage(SelectedMontage);
			return; // 正常に再生されたので終了
		}
	}

	// アニメーション再生
	if (AttackMontage)
	{
		// モンタージュを再生
		PlayAnimMontage(AttackMontage);

		// ※ここでダメージ計算はしません！
		// アニメーション中の「特定の瞬間」に OnAttackHit が呼ばれるのを待ちます。
	}
	else
	{
		// もしモンタージュが設定されていない場合は、仕方ないので即座にダメージを与えます
		OnAttackHit();
	}
}

// アニメーションの「剣が当たった瞬間」に呼ばれる関数
void AMyProject1Character::OnAttackHit()
{
	if (!CurrentTarget) return;

	AMyProject1Character* EnemyChar = Cast<AMyProject1Character>(CurrentTarget);
	if (!EnemyChar) return;

	// MyStats.AttackPower/DefensePowerは常に疲労デバフ込みの値が入っているので、そのまま使えばよい
	// （以前はここで一時的に差し替え→戻す処理をしていたが、RecalculateFatigueAdjustedCombatStatsにより不要になった）

	// 特殊技の倍率・ボーナスをセットする変数を準備
	float SkillMultiplier = 1.0f;
	float SkillCritBonus = 0.0f;

	if (bIsUsingSpecialAttack)
	{
		SkillMultiplier = CurrentExecutingSkillData.DamageMultiplier;
		SkillCritBonus = CurrentExecutingSkillData.CriticalRateBonus;
	}

	// 1. ダメージ計算を実行
	FDamageResult Result = URpgDamageCalculator::CalculateDamage(
		this->MyStats,
		EnemyChar->MyStats,
		SkillMultiplier,
		SkillCritBonus
	);

	// 2. 判定に基づいてSEを選択
	USoundBase* SoundToPlay = nullptr;

	if (Result.bIsHit)
	{
		FJobAttributes* JobData = JobRow.GetRow<FJobAttributes>(JobRow.RowName.ToString());
		if (JobData)
		{
			if (Result.bIsCritical) {
				SoundToPlay = !JobData->CriticalSound.IsNull() ? JobData->CriticalSound.LoadSynchronous() : JobData->AttackSound.LoadSynchronous();
			}
			else {
				SoundToPlay = JobData->AttackSound.LoadSynchronous();
			}
		}
	}
	else
	{
		SoundToPlay = GlobalMissSound;
	}

	// 3. 音を再生
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, GetActorLocation());
	}

	// 攻撃者と防御者の名前を準備しておく
	FString AttackerName = this->MyStats.NPCName.IsEmpty() ? this->CharacterName : this->MyStats.NPCName;
	FString DefenderName = EnemyChar->MyStats.NPCName.IsEmpty() ? EnemyChar->CharacterName : EnemyChar->MyStats.NPCName;

	// --- 3. 命中時の処理（ログ出力・ダメージ適用） ---
	if (Result.bIsHit)
	{
		if (this->IsPlayerControlled())
		{
			// 【自分が殴った場合】
			FString DamageLog;
			if (bIsUsingSpecialAttack) {
				if (Result.bIsCritical) {
					
					DamageLog = FString::Printf(TEXT("%sの【%s】 → %sにクリティカル！ %.0fのダメージ！"), *AttackerName, *CurrentExecutingSkillData.SkillName, *DefenderName, Result.DamageAmount);
				}
				else {
					DamageLog = FString::Printf(TEXT("%sの【%s】 → %sに、%.0fのダメージ！"), *AttackerName, *CurrentExecutingSkillData.SkillName, *DefenderName, Result.DamageAmount);
				}
			}
			else {
				if (Result.bIsCritical) {
					DamageLog = FString::Printf(TEXT("%sにクリティカル！ %.0fのダメージ！"), *DefenderName, Result.DamageAmount);
				}
				else {
					DamageLog = FString::Printf(TEXT("%sに、%.0fのダメージ！"), *DefenderName, Result.DamageAmount);
				}
			}
			this->OnReceiveLogMessage(DamageLog, ELogMessageType::DamageDealt);
		}
		else if (EnemyChar->IsPlayerControlled())
		{
			// 【敵が自分を殴った場合】
			FString TakenLog;
			ELogMessageType SelectedType;

			if (bIsUsingSpecialAttack) {
				if (Result.bIsCritical) {
					
					TakenLog = FString::Printf(TEXT("%sの【%s】 → クリティカル！！ %.0fのダメージ！"), *AttackerName, *CurrentExecutingSkillData.SkillName, Result.DamageAmount);
				}
				else {
					TakenLog = FString::Printf(TEXT("%sの【%s】 → %.0fのダメージ！"), *AttackerName, *CurrentExecutingSkillData.SkillName, Result.DamageAmount);
				}
			}
			else {
				if (Result.bIsCritical) {
					TakenLog = FString::Printf(TEXT("クリティカル！！ %sから、%.0fのダメージ！"), *AttackerName, Result.DamageAmount);
				}
				else {
					TakenLog = FString::Printf(TEXT("%sから、%.0fのダメージ！"), *AttackerName, Result.DamageAmount);
				}
			}
			SelectedType = Result.bIsCritical ? ELogMessageType::Critical : ELogMessageType::DamageTaken;
			EnemyChar->OnReceiveLogMessage(TakenLog, SelectedType);
		}

		// クリティカルかどうかに応じて「ダメージタイプ」を切り替える
		TSubclassOf<UDamageType> DmgTypeClass = Result.bIsCritical ? UCriticalDamageType::StaticClass() : UDamageType::StaticClass();
		UGameplayStatics::ApplyDamage(CurrentTarget, Result.DamageAmount, GetController(), this, DmgTypeClass);
	}
	else
	{
		// --- 4. ミス・回避のログ処理 ---
		if (this->IsPlayerControlled())
		{
			FString MissLog;
			if (bIsUsingSpecialAttack) {
				
				MissLog = FString::Printf(TEXT("%sの【%s】 → %sにミス！"), *AttackerName, *CurrentExecutingSkillData.SkillName, *DefenderName);
			}
			else {
				MissLog = FString::Printf(TEXT("%sの攻撃 → %sにミス！"), *AttackerName, *DefenderName);
			}
			this->OnReceiveLogMessage(MissLog, ELogMessageType::Default);
		}
		else if (EnemyChar->IsPlayerControlled())
		{
			FString EvadeLog;
			if (bIsUsingSpecialAttack) {
				
				EvadeLog = FString::Printf(TEXT("%sの【%s】を回避！"), *AttackerName, *CurrentExecutingSkillData.SkillName);
			}
			else {
				EvadeLog = FString::Printf(TEXT("%sの攻撃を回避！"), *AttackerName);
			}
			EnemyChar->OnReceiveLogMessage(EvadeLog, ELogMessageType::Default);
		}

		UGameplayStatics::ApplyDamage(CurrentTarget, 0.1f, GetController(), this, UDamageType::StaticClass());
	}
}

// ターゲット変更を一括管理する関数
void AMyProject1Character::SetCurrentTarget(AActor* NewTarget)
{
	LastCombatTime = GetWorld()->GetTimeSeconds();

	if (CurrentTarget == NewTarget) return;
	ConsecutiveAttackCount = 0;

	// AIコントローラーを取得
	AAIController* AIC = Cast<AAIController>(GetController());

	if (CurrentTarget)
	{
		// 前のターゲットへの注視を解除
		if (AIC) AIC->ClearFocus(EAIFocusPriority::Gameplay);
		OnTargetUpdated(CurrentTarget, false);
	}

	CurrentTarget = NewTarget;

	if (CurrentTarget)
	{
		OnTargetUpdated(CurrentTarget, true);

		if (AIC && AIC->GetBlackboardComponent())
		{
			AIC->GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), CurrentTarget);
		}

		if (IsPlayerControlled())
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
				{
					if (HUD->TargetCursorSound)
					{
						UGameplayStatics::PlaySound2D(GetWorld(), HUD->TargetCursorSound);
					}
				}
			}
		}
	}
}

void AMyProject1Character::NotifyNearbyAllies(AActor* TargetToAttack)
{
	if (!TargetToAttack) return;

	// 1. 検索するオブジェクトの種類を指定（Pawnを指定）
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

	// 2. 無視するアクターのリスト（自分自身）
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	// 3. 結果を格納する配列
	TArray<AActor*> NearbyActors;

	// 範囲内のアクターを探す
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		LinkRadius,      // リンク範囲
		ObjectTypes,     // ★ここを修正しました
		APawn::StaticClass(),
		IgnoreActors,
		NearbyActors
	);

	// 4. 見つかった仲間にターゲットを教える
	for (AActor* Actor : NearbyActors)
	{
		AMyProject1Character* Ally = Cast<AMyProject1Character>(Actor);

		// 自分と同じクラス（同じ種類の敵）で、まだターゲットがいない場合
		if (Ally && Ally->bCanLink && Ally->GetClass() == this->GetClass() && Ally->CurrentTarget == nullptr)
		{
			Ally->SetCurrentTarget(TargetToAttack);

			// リンクしたことをログで通知
			FString LinkMsg = FString::Printf(TEXT("%sがリンクした！"), *Ally->MyStats.NPCName);
			Ally->OnReceiveLogMessage(LinkMsg, ELogMessageType::System);
		}
	}
}

void AMyProject1Character::StartAutoAttack()
{
	// ターゲットがいる 且つ まだ戦闘中でも準備中でもない場合のみ開始
	if (CurrentTarget && !bIsAutoAttacking && !bIsPreparingAttack)
	{
		// 自分がプレイヤー操作の時だけ、「相手がEnemyか」をチェックして攻撃を制限する
		if (IsPlayerControlled())
		{
			if (!CurrentTarget->ActorHasTag(FName("Enemy")))
			{
				OnReceiveLogMessage(TEXT("この対象には攻撃できません。"), ELogMessageType::System);
				return; // ここで処理を強制終了
			}
		}

		bIsPreparingAttack = true;
		StartAttackTimestamp = GetWorld()->GetTimeSeconds();

		// オートアタック開始時に疲労度を上げる
		UpdateEnergy(FatigueIncreasePerAttack);

		// 抜刀アニメーションの再生
		if (UnsheatheMontage)
		{
			PlayAnimMontage(UnsheatheMontage);
		}

		// 即座に敵の方を向く準備（Tickを待たずにフラグを折る）
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}

		// 戦闘曲に切り替え！
		if (IsPlayerControlled() && MusicComp)
		{
			MusicComp->SetCombatMusicActive(true);
		}

		// ★このログ送信をコメントアウトします
		// OnReceiveLogMessage(TEXT("オートアタック開始！"), ELogMessageType::System);
	}
}

void AMyProject1Character::ZoomCamera(const FInputActionValue& Value)
{
	float ScrollValue = Value.Get<float>();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// 右クリック中はズーム、それ以外はログスクロール
	if (PC->IsInputKeyDown(EKeys::RightMouseButton))
	{
		if (CameraBoom && ScrollValue != 0.0f)
		{
			float NewArmLength = CameraBoom->TargetArmLength + (ScrollValue * -ZoomStep);
			CameraBoom->TargetArmLength = FMath::Clamp(NewArmLength, MinTargetArmLength, MaxTargetArmLength);
		}
	}
	else
	{
		// 1. ログをスクロールさせる
		OnScrollLog(ScrollValue);

		// 2. 3秒後に「一番下に戻す」合図を送るタイマーをセット
		// 回すたびにリセットされるので、指を止めてから3秒後に発動します
		GetWorldTimerManager().SetTimer(LogScrollTimerHandle, this, &AMyProject1Character::HandleLogAutoScroll, 3.0f, false);
	}
}

void AMyProject1Character::HandleLogAutoScroll()
{
	// WBP（ウィジェット）へ「戻って！」と放送する
	OnLogScrollToBottomDelegate.Broadcast();
}

// --- 1. 経験値を加算する ---
void AMyProject1Character::AddExperience(int32 Amount)
{
	if (!MyStats.bCanLevelUp) return;

	MyStats.CurrentXP += Amount;

	FString ExpMsg = FString::Printf(TEXT("%dポイントの経験値を獲得！"), Amount);
	OnReceiveLogMessage(ExpMsg, ELogMessageType::ExpGain);

	// 必要経験値を超えている間、レベルアップを繰り返す
	while (MyStats.CurrentXP >= MyStats.MaxXP)
	{
		LevelUp();
	}
}

// --- 2. レベルアップ処理 ---
void AMyProject1Character::LevelUp()
{
	MyStats.Level++;

	// 余った経験値を次に持ち越す
	MyStats.CurrentXP -= MyStats.MaxXP;
		
	// ★重要：ステータスの再計算を ApplyJobData に任せる
	ApplyJobData();

	// NPCName（またはCharacterName）を使って、誰がレベルアップしたかを含める
	FString SpeakerName = MyStats.NPCName.IsEmpty() ? CharacterName : MyStats.NPCName;
	FString LevelUpMsg = FString::Printf(TEXT("%sはレベル%dに上がった！"), *SpeakerName, MyStats.Level);

	// ログの種類はシステムメッセージとして送信
	OnReceiveLogMessage(LevelUpMsg, ELogMessageType::System);
}

// --- 3. ジョブデータとレベルに基づいたステータス確定 ---
void AMyProject1Character::ApplyJobData()
{
	if (JobRow.IsNull()) return;

	FJobAttributes* JobData = JobRow.GetRow<FJobAttributes>(JobRow.RowName.ToString());

	if (JobData)
	{
		// --- A. アニメーションと武器メッシュの更新 ---
		if (JobData->AttackMontages.Num() > 0)
		{
			AttackMontage = JobData->AttackMontages[0];
		}

		if (!JobData->CharacterMesh.IsNull())
		{
			GetMesh()->SetSkeletalMesh(JobData->CharacterMesh.LoadSynchronous());
		}

		if (HairMeshComp)
		{
			// 1. DT_Jobs に髪のメッシュがセットされているかチェック（未設定なら完全に無視）
			if (!JobData->HairMesh.IsNull())
			{
				// 2. プレイヤーが「装備品」として別の髪型を適用しているかチェック
				// （装備枠の Hair に何も入っていない場合のみ、ジョブのデフォルト髪型を適用する）
				if (!CurrentEquippedItems.Contains(EEquipmentSlot::Hair))
				{
					HairMeshComp->SetSkeletalMesh(JobData->HairMesh.LoadSynchronous());
				}
			}
		}

		if (FaceMeshComp)
		{
			// DT_Jobs に顔メッシュがセットされているかチェック（未設定なら完全に無視）
			if (!JobData->FaceMesh.IsNull())
			{
				FaceMeshComp->SetSkeletalMesh(JobData->FaceMesh.LoadSynchronous());
			}
		}

		if (WeaponMeshComp)
		{
			if (!JobData->WeaponMesh.IsNull())
			{
				USkeletalMesh* NewMesh = JobData->WeaponMesh.LoadSynchronous();
				WeaponMeshComp->SetSkeletalMesh(NewMesh);

				// 大きさを反映
				WeaponMeshComp->SetRelativeScale3D(JobData->WeaponScale);

				// 強制的に手のソケットにアタッチ
				WeaponMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, JobData->WeaponSocketName);
			}
			else
			{
				WeaponMeshComp->SetSkeletalMesh(nullptr);
			}
		}

		// --- B. StaticMesh (バットなどの置物武器) の処理 ---
		if (StaticWeaponMeshComp)
		{
			if (!JobData->StaticWeaponMesh.IsNull())
			{
				UStaticMesh* NewStaticMesh = JobData->StaticWeaponMesh.LoadSynchronous();
				StaticWeaponMeshComp->SetStaticMesh(NewStaticMesh);

				// ★重要：ここを SkeletalMesh の外に出しました
				// 大きさを反映
				StaticWeaponMeshComp->SetRelativeScale3D(JobData->WeaponScale);

				// ★重要：ここで強制的に手のソケットに吸着（Snap）させる
				StaticWeaponMeshComp->AttachToComponent(
					GetMesh(),
					FAttachmentTransformRules::SnapToTargetNotIncludingScale,
					JobData->WeaponSocketName
				);

				// ★ここからデバッグコードを追加：本当にソケットはあるか？★
				if (!GetMesh()->DoesSocketExist(JobData->WeaponSocketName))
				{
					// 画面の左上に赤いエラー文字を出す
					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("エラー：hand_r_socketが見つかりません！"));
				}
				// ★デバッグコードここまで★
			}
			else
			{
				StaticWeaponMeshComp->SetStaticMesh(nullptr);
			}
		}

		if (JobData->AnimBlueprintClass)
		{
			GetMesh()->SetAnimInstanceClass(JobData->AnimBlueprintClass);

			// 新しく設定されたAnimInstanceのモンタージュ終了イベントを監視する
			UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
			if (AnimInst && !AnimInst->OnMontageEnded.IsAlreadyBound(this, &AMyProject1Character::OnMontageEnded))
			{
				AnimInst->OnMontageEnded.AddDynamic(this, &AMyProject1Character::OnMontageEnded);
			}
		}

		
		// --- ★ B. 再計算前の状態を記録しておく（ここを追加！） ---
		// 現在のHPが最大値以上（＝満タン）かどうかを記憶
		bool bWasFullHP = (MyStats.HP >= MyStats.MaxHP);
		// 疲労度が初期値のままかどうかを記憶
		bool bWasFullEnergy = (MyStats.Energy <= MyStats.BaseEnergy);

		PersonalLootTable = JobData->BaseLootTable;


		// --- C. ステータスの再計算 ---
		MyStats.MaxHP = JobData->BaseHP + ((MyStats.Level - 1) * 15.0f);
		MyStats.STR = JobData->BaseSTR + ((MyStats.Level - 1) * 2.0f);
		MyStats.VIT = JobData->BaseVIT + ((MyStats.Level - 1) * 2.0f);
		MyStats.DEX = JobData->BaseDEX + ((MyStats.Level - 1) * 2.0f);
		MyStats.AGI = JobData->BaseAGI + ((MyStats.Level - 1) * 2.0f);

		// ※AttackPower/DefensePowerはこの直後のRefreshEquipmentStats()がBaseAttackPower/BaseDefensePowerから
		//   疲労補正込みで確定させるため、ここではBase側だけ更新しておく
		MyStats.BaseAttackPower = MyStats.STR * 2.0f;
		MyStats.BaseDefensePower = MyStats.VIT * 2.0f;
		MyStats.Accuracy = MyStats.DEX * 1.5f;
		MyStats.Evasion = MyStats.AGI * 1.5f;

		// --- ★ D. 現在値の同期 ---
		if (bWasFullHP)
		{
			// 元々満タンだった場合（新規POP時など）は、新しい最大値でも満タンにする
			MyStats.HP = MyStats.MaxHP;
		}
		else
		{
			// ダメージを受けていた場合（レベルアップ時、エリア移動時など）は現在値を維持する
			// ※ただし、新しい最大値をはみ出さないようにだけ補正する
			if (MyStats.HP > MyStats.MaxHP) MyStats.HP = MyStats.MaxHP;
		}

		if (bWasFullEnergy)
		{
			MyStats.Energy = MyStats.BaseEnergy;
		}
		else
		{
			// 疲労度がベースを下回らないように補正
			if (MyStats.Energy < MyStats.BaseEnergy) MyStats.Energy = MyStats.BaseEnergy;
		}
	}

	// --- E. 通知とログの更新 ---
	if (!MyStats.NPCName.IsEmpty())
	{
		CharacterName = MyStats.NPCName;
	}
	UpdateHealthWidgetName(CharacterName);

	RefreshEquipmentStats();

	if (OnHPChangedDelegate.IsBound())
	{
		OnHPChangedDelegate.Broadcast(MyStats.HP, MyStats.MaxHP);
	}
	OnHPChanged(MyStats.HP, MyStats.MaxHP);

	NotifyStatsChanged();
}

// --- 代行者クラスの等級：ギルドNPCへの申請による昇格判定 ---
bool AMyProject1Character::SetAdventurerRank(FName TargetRankRowName)
{
	// 昇格条件はここでは判定しない（呼び出し側＝昇格QuestのRequiredStats/RequiredFlagで
	// 「受注できるかどうか」を制御し、そのQuestの報告ダイアログからここが呼ばれる想定のため）
	const UEnum* RankEnum = StaticEnum<EAdventurerRank>();
	const int64 RankValue = RankEnum->GetValueByName(TargetRankRowName);
	if (RankValue == INDEX_NONE)
	{
		OnReceiveLogMessage(FString::Printf(TEXT("代行者等級「%s」は存在しません。"), *TargetRankRowName.ToString()), ELogMessageType::System);
		return false;
	}

	MyStats.AdventurerRank = static_cast<EAdventurerRank>(RankValue);

	// ボーナスは任意：AdventurerRankDataTableに同名の行があれば加算する（無ければ等級だけ変わる）
	if (AdventurerRankDataTable)
	{
		if (FAdventurerRankData* RankData = AdventurerRankDataTable->FindRow<FAdventurerRankData>(TargetRankRowName, TEXT("SetAdventurerRank")))
		{
			for (const FEquipmentStatModifier& Bonus : RankData->RankUpBonuses)
			{
				if (Bonus.TargetStat == ETargetStat::CustomExtraStat)
				{
					if (!Bonus.ExtraStatName.IsNone())
					{
						MyStats.ExtraStats.FindOrAdd(Bonus.ExtraStatName) += Bonus.Amount;
					}
					continue;
				}

				switch (Bonus.TargetStat)
				{
				case ETargetStat::STR:          MyStats.STR += Bonus.Amount;          break;
				case ETargetStat::DEX:          MyStats.DEX += Bonus.Amount;          break;
				case ETargetStat::VIT:          MyStats.VIT += Bonus.Amount;          break;
				case ETargetStat::AGI:          MyStats.AGI += Bonus.Amount;          break;
				case ETargetStat::Accuracy:     MyStats.Accuracy += Bonus.Amount;     break;
				case ETargetStat::Evasion:      MyStats.Evasion += Bonus.Amount;      break;
				case ETargetStat::AttackPower:  MyStats.BaseAttackPower += Bonus.Amount;  break;
				case ETargetStat::DefensePower: MyStats.BaseDefensePower += Bonus.Amount; break;
				case ETargetStat::Stamina:      MyStats.Stamina += Bonus.Amount;      break;
				case ETargetStat::HP:           MyStats.MaxHP += Bonus.Amount;        break;
				case ETargetStat::Favor:        MyStats.Favor += Bonus.Amount;        break;
				case ETargetStat::Fame:         MyStats.Fame += Bonus.Amount;         break;
				case ETargetStat::Charm:        MyStats.Charm += Bonus.Amount;        break;
				case ETargetStat::Alcohol:      MyStats.Alcohol += Bonus.Amount;      break;
				case ETargetStat::Mental:       MyStats.MentalBonus += Bonus.Amount;  break;
				default: break;
				}
			}
		}
	}

	FString SpeakerName = MyStats.NPCName.IsEmpty() ? CharacterName : MyStats.NPCName;
	OnReceiveLogMessage(FString::Printf(TEXT("%sは代行者等級「%s」になった！"), *SpeakerName, *GetAdventurerRankDisplayName().ToString()), ELogMessageType::System);

	if (OnAdventurerRankChangedDelegate.IsBound())
	{
		OnAdventurerRankChangedDelegate.Broadcast();
	}

	// BaseAttackPower/BaseDefensePowerが変わった可能性があるので、疲労補正込みの表示値を再計算する
	RecalculateFatigueAdjustedCombatStats();
	NotifyStatsChanged();

	return true;
}

FText AMyProject1Character::GetAdventurerRankDisplayName() const
{
	const UEnum* RankEnum = StaticEnum<EAdventurerRank>();
	return RankEnum->GetDisplayNameTextByValue(static_cast<int64>(MyStats.AdventurerRank));
}

bool AMyProject1Character::TryPerformAutoAttack()
{
	if (!CurrentTarget || IsDead()) return false;

	// 特殊技のモーション中であれば、通常攻撃のタイマー判定ごとブロックする
	if (bIsUsingSpecialAttack)
	{
		return false;
	}

	// 1. まだ納刀状態なら抜刀を開始する
	if (!bIsAutoAttacking && !bIsPreparingAttack)
	{
		StartAutoAttack();
		// ここで true を返すことで、BTタスクに「準備を開始した（成功）」と伝え
		// BTが次の Wait ノードに進めるようにします
		return true;
	}

	// 2. 抜刀準備中（2.5秒待機中）も「成功（進行中）」を返し、ループを安定させます
	if (bIsPreparingAttack)
	{
		return true;
	}

	// 3. 完全に抜刀完了（bIsAutoAttacking == true）している時だけ攻撃処理へ
	double CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - LastAttackTime >= GetModifiedAttackSpeed())
	{
		// まず特殊技が撃てるかチェック！
		if (TryUseSpecialAttack())
		{
			// 特殊技が発動したので、通常攻撃はせずに終了
			return true;
		}

		// 特殊技が出なかった場合は、今まで通り通常攻撃を行う
		PerformAutoAttack();

		// オートアタックした回数を+1する
		ConsecutiveAttackCount++;

		LastAttackTime = CurrentTime;
		return true; // 攻撃成功
		
	}

	return false; // リキャスト中
}

// --- 特殊技の発動 ---
void AMyProject1Character::PerformSpecialAttack(UAnimMontage* SpecialMontage)
{
	if (!SpecialMontage || IsDead()) return;

	// 特殊技中のフラグを立てる
	bIsUsingSpecialAttack = true;
	ActiveSpecialMontage = SpecialMontage;

	// 特殊技のモンタージュを再生
	PlayAnimMontage(SpecialMontage);

	// 必要であればここに「○○を構えた！」などのログ処理を追加
}

// --- モンタージュ終了時の処理 ---
void AMyProject1Character::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 終わったモンタージュが、記憶しておいた「特殊技」と完全に一致した時だけフラグを折る
	if (bIsUsingSpecialAttack && Montage == ActiveSpecialMontage)
	{
		bIsUsingSpecialAttack = false;
		ActiveSpecialMontage = nullptr; // 記憶をリセット

		// 特殊技が終わった瞬間にディレイタイマーをリセット
		LastAttackTime = GetWorld()->GetTimeSeconds();
	}
}

void AMyProject1Character::HandleTargetDeath()
{
	// 1. 次の最適なターゲットを探す（以前作成した関数を流用）
	AActor* NextTarget = FindBestNextTarget();

	if (NextTarget)
	{
		// 次の敵がいる場合：
		SetCurrentTarget(NextTarget);

		// リターゲット直後は、既存の「抜刀準備」の仕組み（AttackStartupDelay＝2.5秒）を
		// そのまま流用して攻撃までのディレイを挟む（抜刀モーションは既に済んでいるので再生はしない）
		bIsAutoAttacking = false;
		bIsPreparingAttack = true;
		StartAttackTimestamp = GetWorld()->GetTimeSeconds();


	}
	else
	{
		// 次の敵がいない場合：
		SetCurrentTarget(nullptr);
		bIsAutoAttacking = false;
		bIsPreparingAttack = false;
		
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
			{
				// 1. 万が一インベントリ画面が開いていたら、先に閉じる
				// (これをしないと、インベントリが閉じつつコマンドメニューが開いたままになる可能性があります)
				if (HUD->InventoryMenuWidget && HUD->InventoryMenuWidget->IsInViewport())
				{
					HUD->ToggleInventoryMenu();
				}

				// 2. コマンドメニューが開いていたら、閉じる
				// IsInViewport() は表示中なら true を返すので、Toggleを呼べば「閉じる処理」が走ります
				if (HUD->CommandMenuWidget && HUD->CommandMenuWidget->IsInViewport())
				{
					HUD->ToggleCommandMenu();
				}
			}
		}
	}
}

AActor* AMyProject1Character::FindBestNextTarget()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyProject1Character::StaticClass(), FoundActors);

	AActor* ClosestAggroTarget = nullptr; // 自分を狙っている敵用
	float MinDistAggro = TargetingRange;

	for (AActor* Actor : FoundActors)
	{
		AMyProject1Character* PotentialEnemy = Cast<AMyProject1Character>(Actor);

		// 自分以外、かつ生きていて、射程内にいるか
		if (!PotentialEnemy || PotentialEnemy == this || PotentialEnemy->IsDead()) continue;
		if (!PotentialEnemy->ActorHasTag(FName("Enemy"))) continue;

		float Dist = GetDistanceTo(PotentialEnemy);
		if (Dist > TargetingRange) continue;

		// 自分をターゲットにしている（＝自分を攻撃してきている）敵だけを対象にする
		if (PotentialEnemy->CurrentTarget == this)
		{
			if (Dist < MinDistAggro)
			{
				MinDistAggro = Dist;
				ClosestAggroTarget = PotentialEnemy;
			}
		}
	}

	// 自分を攻撃してきている敵がいなければ、リターゲットしない（nullptrを返す）
	return ClosestAggroTarget;
}

void AMyProject1Character::TalkToLog(const FString& Message)
{
	// 1. NPCの名前を取得（空っぽなら"???"にする）
	FString SpeakerName = MyStats.NPCName.IsEmpty() ? TEXT("???") : MyStats.NPCName;

	// 2. 「名前 : メッセージ」の形に整形する
	// 例：「Shopkeeper : いらっしゃいませ！」
	FString FormattedMsg = FString::Printf(TEXT("%s : %s"), *SpeakerName, *Message);

	// 3. 既に作成済みのログ送信イベントを呼び出す
	OnReceiveLogMessage(FormattedMsg, ELogMessageType::Dialogue);
}

void AMyProject1Character::ApplyItemBuff(FString ItemName, UTexture2D* Icon, const TArray<FItemEffect>& Effects, float Duration)
{
	if (bIsDead || Duration <= 0.0f) return;

	// 効果のどれか1つでも「重複可」がチェックされていれば、このアイテムは重ねがけを許可する
	const bool bAllowStacking = Effects.ContainsByPredicate([](const FItemEffect& Effect) { return Effect.bAllowStacking; });

	if (!bAllowStacking)
	{
		for (const FActiveBuff& ActiveBuff : ActiveBuffs)
		{
			// 今使おうとしているアイテム名が、すでにリストに存在する場合
			if (ActiveBuff.BuffName == ItemName)
			{
				// ログに「効果なし」のメッセージを出す
				FString Msg = FString::Printf(TEXT("%sの効果はすでに発動しているため効果なし。"), *ItemName);
				if (IsPlayerControlled())
				{
					OnReceiveLogMessage(Msg, ELogMessageType::System);
				}

				// returnでここで処理を強制終了する（ステータス加算もアイコン追加も起きない）
				return;
			}
		}
	}

	// 今回の使用分を識別するID（重複可アイテムを連続使用した時、期限切れ処理で
	// 「今回の使用分」だけを正しく取り除くために使う）
	const int32 ThisStackID = NextBuffStackID++;
	const float NewExpirationTime = GetWorld()->GetTimeSeconds() + Duration;

	bool bHasAddedBuffIcon = false;

	// 重複可（bAllowStacking）アイテムの場合、ステータス効果は使用ごとに独立して積み重ねる（各々が自分の
	// タイマーで個別に切れる）が、表示アイコンまで使った分だけ増えると煩わしいので、同じ見た目のアイコンが
	// 既にあれば増やさず、一番遅く切れる使用分にアイコンの消去タイミングを付け替えるだけにする。
	auto AddOrRefreshBuffIcon = [&](UTexture2D* IconToUse)
	{
		if (bAllowStacking)
		{
			for (FActiveBuff& ExistingBuff : ActiveBuffs)
			{
				if (ExistingBuff.BuffName == ItemName && ExistingBuff.BuffIcon == IconToUse)
				{
					if (NewExpirationTime >= ExistingBuff.ExpirationTime)
					{
						ExistingBuff.ExpirationTime = NewExpirationTime;
						ExistingBuff.StackID = ThisStackID; // アイコンの消去は今回の使用分の期限切れに委ねる
					}
					bHasAddedBuffIcon = true;
					return;
				}
			}
		}

		FActiveBuff NewBuff;
		NewBuff.BuffName = ItemName; // 削除時の目印としてアイテム名を記録
		NewBuff.BuffIcon = IconToUse;
		NewBuff.ExpirationTime = NewExpirationTime;
		NewBuff.StackID = ThisStackID;
		ActiveBuffs.Add(NewBuff);
		bHasAddedBuffIcon = true;
	};

	// 1. すべての効果を一度に適用し、必要なら専用バフアイコンを登録する
	for (const FItemEffect& Effect : Effects)
	{
		switch (Effect.TargetStat)
		{
		case ETargetStat::Accuracy:    MyStats.Accuracy += Effect.EffectAmount;    break;
		case ETargetStat::STR:         MyStats.STR += Effect.EffectAmount;         break;
		case ETargetStat::DEX:         MyStats.DEX += Effect.EffectAmount;         break;
		case ETargetStat::VIT:         MyStats.VIT += Effect.EffectAmount;         break;
		case ETargetStat::AGI:         MyStats.AGI += Effect.EffectAmount;         break;
		case ETargetStat::Evasion:     MyStats.Evasion += Effect.EffectAmount;     break;
		case ETargetStat::AttackPower: MyStats.BaseAttackPower += Effect.EffectAmount; break;
		case ETargetStat::DefensePower: MyStats.BaseDefensePower += Effect.EffectAmount; break;
		case ETargetStat::Stamina:     MyStats.Stamina += Effect.EffectAmount;     break;
		case ETargetStat::Alcohol:     MyStats.Alcohol += Effect.EffectAmount;     break;
		case ETargetStat::Fame:        MyStats.Fame += Effect.EffectAmount;        break;
		case ETargetStat::Favor:       MyStats.Favor += Effect.EffectAmount;       break;
		case ETargetStat::Charm:       MyStats.Charm += Effect.EffectAmount;       break;
		case ETargetStat::Mental:      MyStats.Mental += Effect.EffectAmount;      break;
		default: break;
		}

		// ★追加：バフデータテーブルがセットされており、BuffIDが空ではない場合
		if (BuffDataTable && !Effect.BuffID.IsNone())
		{
			// データテーブルから、指定されたIDのバフデータを検索
			FBuffData* BuffRow = BuffDataTable->FindRow<FBuffData>(Effect.BuffID, TEXT("BuffLookup"));
			if (BuffRow)
			{
				AddOrRefreshBuffIcon(BuffRow->BuffIcon); // ★アイテム画像ではなく、バフ専用画像をセット！
			}
		}
	}

	// 万が一、データテーブルが未設定だったり、BuffIDを書き忘れていた場合の保険（今まで通りアイテムのアイコンを使う）
	if (!bHasAddedBuffIcon && Icon)
	{
		AddOrRefreshBuffIcon(Icon);
	}

	// BaseAttackPower/BaseDefensePowerが変わった可能性があるので、疲労補正込みの表示値を再計算する
	RecalculateFatigueAdjustedCombatStats();

	// 2. タイマーを1つだけセットする
	FTimerHandle TimerHandle;
	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &AMyProject1Character::ExpireItemBuff, ItemName, Effects, ThisStackID);
	GetWorldTimerManager().SetTimer(TimerHandle, Delegate, Duration, false);

	if (OnBuffListChangedDelegate.IsBound())
	{
		OnBuffListChangedDelegate.Broadcast();
	}
}

void AMyProject1Character::ExpireItemBuff(FString ItemName, TArray<FItemEffect> Effects, int32 StackID)
{
	// 3. すべての効果を元に戻す
	for (const FItemEffect& Effect : Effects)
	{
		switch (Effect.TargetStat)
		{
		case ETargetStat::Accuracy:    MyStats.Accuracy -= Effect.EffectAmount;    break;
		case ETargetStat::STR:         MyStats.STR -= Effect.EffectAmount;         break;
		case ETargetStat::DEX:         MyStats.DEX -= Effect.EffectAmount;         break;
		case ETargetStat::VIT:         MyStats.VIT -= Effect.EffectAmount;         break;
		case ETargetStat::AGI:         MyStats.AGI -= Effect.EffectAmount;         break;
		case ETargetStat::Evasion:     MyStats.Evasion -= Effect.EffectAmount;     break;
		case ETargetStat::AttackPower: MyStats.BaseAttackPower -= Effect.EffectAmount; break;
		case ETargetStat::DefensePower: MyStats.BaseDefensePower -= Effect.EffectAmount; break;
		case ETargetStat::Stamina:     MyStats.Stamina -= Effect.EffectAmount;     break;
		case ETargetStat::Alcohol:     MyStats.Alcohol -= Effect.EffectAmount;     break;
		case ETargetStat::Fame:        MyStats.Fame -= Effect.EffectAmount;        break;
		case ETargetStat::Favor:       MyStats.Favor -= Effect.EffectAmount;       break;
		case ETargetStat::Charm:       MyStats.Charm -= Effect.EffectAmount;       break;
		case ETargetStat::Mental:      MyStats.Mental -= Effect.EffectAmount;      break;
		default: break;
		}
	}

	// BaseAttackPower/BaseDefensePowerが変わった可能性があるので、疲労補正込みの表示値を再計算する
	RecalculateFatigueAdjustedCombatStats();

	// --- 追加：バフリストから削除してUIに通知 ---
	// ★ポイント：1つのアイテムから複数のバフアイコンが出ている可能性がある上、
	// 重複可アイテムだと同名バフが複数同時に存在しうるので、名前ではなく
	// 「今回の使用分」を示すStackIDが一致するものだけを消去します。
	for (int32 i = ActiveBuffs.Num() - 1; i >= 0; --i)
	{
		if (ActiveBuffs[i].StackID == StackID)
		{
			ActiveBuffs.RemoveAt(i);
		}
	}

	if (OnBuffListChangedDelegate.IsBound())
	{
		OnBuffListChangedDelegate.Broadcast();
	}

	FString Msg = FString::Printf(TEXT("%sの効果が切れた。"), *ItemName);
	OnReceiveLogMessage(Msg, ELogMessageType::System);
}

void AMyProject1Character::PlayFootstepSound()
{
	// 1. 足元へ向かって短いレイ（線）を飛ばす
	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0, 0, 150.0f); // 1.5m下までチェック

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bReturnPhysicalMaterial = true; // 物理材質を取得するフラグを立てる

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		// 2. 地面の物理材質を取得
		UPhysicalMaterial* PhysMat = HitResult.PhysMaterial.Get();
		if (PhysMat)
		{
			EPhysicalSurface SurfaceType = PhysMat->SurfaceType;

			// 3. 材質に一致する音を探して再生
			if (FootstepSounds.Contains(SurfaceType))
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					FootstepSounds[SurfaceType],
					HitResult.Location,
					FRotator::ZeroRotator,
					1.0f,   // 音量
					1.0f,   // ピッチ
					0.0f,   // 開始時間
					FootstepAttenuation);

				MakeNoise(1.0f, this, HitResult.Location);

			}
		}
	}
}

bool AMyProject1Character::IsReadingOldLogs() const
{
	// 3秒タイマーが動いている間は「読んでいる最中」とみなす
	return GetWorldTimerManager().IsTimerActive(LogScrollTimerHandle);
}

void AMyProject1Character::HandleAutoRecovery()
{
	// 1. 死んでいたら回復しない
	if (IsDead()) return;

	// 2. 戦闘中（ターゲットがいる状態）なら回復しない
	// ※「抜刀中のみ回復しない」にしたい場合は bIsAutoAttacking をチェックしてください
	if (CurrentTarget != nullptr) return;

	// 3. HPが満タンなら何もしない
	if (MyStats.HP >= MyStats.MaxHP) return;

	double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastCombatTime < AutoRecoveryStartDelay)
	{
		return;
	}

	// 4. 回復量を計算（最大HP × ％）
	float RecoverAmount = MyStats.MaxHP * AutoRecoveryRate;

	// 少なくとも1は回復させる
	if (RecoverAmount < 1.0f) RecoverAmount = 1.0f;

	// 5. HPを回復させる（ログは出さない設定にしていますが必要なら追加可）
	UpdateHealth(RecoverAmount);

	// デバッグ用：回復したか確認したい場合のみコメントアウトを外す
	// UE_LOG(LogTemp, Log, TEXT("Auto Recovered: %.1f"), RecoverAmount);
}

void AMyProject1Character::NotifyStatsChanged()
{
	// UI側で「聞く準備（Bind）」ができているか確認してから合図を送る
	if (OnStatsUpdatedDelegate.IsBound())
	{
		OnStatsUpdatedDelegate.Broadcast();
	}
}

void AMyProject1Character::UpdateEnergy(float Amount)
{
	if (IsDead() || !IsPlayerControlled()) return;

	float OldEnergy = MyStats.Energy;

	// Energyは「BaseEnergy(蓄積値)」から「MaxEnergy(100)」の範囲に制限する
	MyStats.Energy = FMath::Clamp(MyStats.Energy + Amount, MyStats.BaseEnergy, MyStats.MaxEnergy);

	// もし値が変動していたら、疲労段階（AttackPower/DefensePower）も再計算してUIに合図を送る
	if (MyStats.Energy != OldEnergy)
	{
		RecalculateFatigueAdjustedCombatStats();
		NotifyStatsChanged();
	}
}

void AMyProject1Character::HandleFatigueTick()
{
	if (IsDead() || !IsPlayerControlled()) return;

	float OldEnergy = MyStats.Energy;

	// --- 1. GameInstanceの時間の進み具合を取得 ---
	float InGameDaysPassed = 0.0f; // この1秒間（1Tick）で進んだゲーム内の「日数」

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		if (GameInst->RealSecondsPerGameMinute > 0.0f) // 0割り防止
		{
			// 現実の1秒で「ゲーム内の何分」が進むか
			float InGameMinutesPassed = 1.0f / GameInst->RealSecondsPerGameMinute;

			// 1日は1440分なので、分を1440で割って「日数」に変換
			InGameDaysPassed = InGameMinutesPassed / 1440.0f;
		}
	}

	// --- 2. 蓄積疲労度の増加（ゲーム内1日単位に連動） ---
	// 「1日あたりの増加量(20)」 × 「実際に進んだ日数」
	float BaseIncreaseRate = FatigueIncreasePerInGameDay * InGameDaysPassed;
	MyStats.BaseEnergy = FMath::Clamp(MyStats.BaseEnergy + BaseIncreaseRate, 0.0f, MyStats.MaxEnergy);

	// Energyが蓄積値を下回らないように強制的に押し上げる
	if (MyStats.Energy < MyStats.BaseEnergy)
	{
		MyStats.Energy = MyStats.BaseEnergy;
	}

	// --- 3. 疲労度の自然変動（戦闘時の増減はリアルタイム基準） ---
	if (bIsAutoAttacking || bIsPreparingAttack)
	{
		// 抜刀中（エンゲージ中）：1秒あたりの疲労度を加算
		MyStats.Energy = FMath::Clamp(MyStats.Energy + FatigueIncreasePerSec, MyStats.BaseEnergy, MyStats.MaxEnergy);
	}
	else
	{
		// 非戦闘中：最後に戦闘してから一定時間経ったら回復（減少）開始
		double CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastCombatTime >= AutoRecoveryStartDelay)
		{
			MyStats.Energy = FMath::Clamp(MyStats.Energy - FatigueDecreasePerSec, MyStats.BaseEnergy, MyStats.MaxEnergy);
		}
	}

	// 今回の1秒間で値が少しでも変わっていたら、疲労段階（AttackPower/DefensePower）も再計算してUIを更新する
	if (MyStats.Energy != OldEnergy)
	{
		RecalculateFatigueAdjustedCombatStats();
		NotifyStatsChanged();
	}
}

// --- 待機/睡眠による時間スキップ分の蓄積疲労度を進める（睡眠時は逆に回復させる） ---
void AMyProject1Character::ApplyFatigueForSkippedMinutes(int32 MinutesSkipped, bool bIsSleep)
{
	if (MinutesSkipped <= 0 || IsDead() || !IsPlayerControlled()) return;

	float OldEnergy = MyStats.Energy;

	if (bIsSleep)
	{
		// 睡眠：1時間あたりMaxEnergyのFatigueDecreasePercentPerSleepHour(%)分だけ蓄積疲労度を下げる
		float HoursSlept = MinutesSkipped / 60.0f;
		float DecreaseAmount = (FatigueDecreasePercentPerSleepHour / 100.0f) * MyStats.MaxEnergy * HoursSlept;

		MyStats.BaseEnergy = FMath::Clamp(MyStats.BaseEnergy - DecreaseAmount, 0.0f, MyStats.MaxEnergy);

		// EnergyもBaseEnergyと同じ分だけ下げる（BaseEnergyを下回らない範囲で）
		MyStats.Energy = FMath::Clamp(FMath::Max(MyStats.Energy - DecreaseAmount, MyStats.BaseEnergy), 0.0f, MyStats.MaxEnergy);
	}
	else
	{
		// HandleFatigueTickと同じ「1日あたりの増加量 × 進んだ日数」の計算を、スキップした分だけまとめて適用する
		float InGameDaysPassed = MinutesSkipped / 1440.0f;
		float BaseIncreaseRate = FatigueIncreasePerInGameDay * InGameDaysPassed;
		MyStats.BaseEnergy = FMath::Clamp(MyStats.BaseEnergy + BaseIncreaseRate, 0.0f, MyStats.MaxEnergy);

		// Energyが蓄積値を下回らないように強制的に押し上げる
		if (MyStats.Energy < MyStats.BaseEnergy)
		{
			MyStats.Energy = MyStats.BaseEnergy;
		}
	}

	if (MyStats.Energy != OldEnergy)
	{
		RecalculateFatigueAdjustedCombatStats();
		NotifyStatsChanged();
	}
}

// --- 疲労度を加味した攻撃速度（間隔）の計算 ---
float AMyProject1Character::GetModifiedAttackSpeed() const
{
	float BaseSpeed = AttackSpeed;

	if (!IsPlayerControlled()) return BaseSpeed;

	// AttackSpeedは「攻撃間隔（秒）」なので、数値を大きくすると攻撃が遅くなります
	if (MyStats.Energy >= FatigueThreshold2)
	{
		// デバフ②：速度5%ダウン（＝攻撃間隔が5%長くなる）
		return BaseSpeed * (1.0f + FatigueSpeedPenalty2);
	}

	// 90未満なら速度ペナルティ無し
	return BaseSpeed;
}

// --- 疲労度に応じて、MyStats.AttackPower/DefensePowerをBase側から直接再計算する ---
// （ステータス画面はAttackPower/DefensePowerを直接表示しているだけなので、これで表示にも自動的に反映される）
void AMyProject1Character::RecalculateFatigueAdjustedCombatStats()
{
	float AttackFactor = 1.0f;
	float DefenseFactor = 1.0f;

	// 現在の疲労段階に応じて画面上部に出すバフのBuffID（段階に該当しなければNAME_None＝非表示）
	FName TargetFatigueBuffID = NAME_None;

	// NPC/敵には疲労デバフを適用しない（プレイヤーのみ）
	if (IsPlayerControlled())
	{
		if (MyStats.Energy >= FatigueThreshold2)
		{
			// デバフ③（重度）：90以上
			AttackFactor = 1.0f - FatigueAttackPenalty2;
			DefenseFactor = 1.0f - FatigueDefensePenalty2;
			TargetFatigueBuffID = FatigueBuffID2;
		}
		else if (MyStats.Energy >= FatigueThresholdMid)
		{
			// デバフ②（中度）：75以上
			AttackFactor = 1.0f - FatigueAttackPenaltyMid;
			DefenseFactor = 1.0f - FatigueDefensePenaltyMid;
			TargetFatigueBuffID = FatigueBuffIDMid;
		}
		else if (MyStats.Energy >= FatigueThreshold1)
		{
			// デバフ①（軽度）：50以上
			AttackFactor = 1.0f - FatigueAttackPenalty1;
			DefenseFactor = 1.0f - FatigueDefensePenalty1;
			TargetFatigueBuffID = FatigueBuffID1;
		}
	}

	float NewAttackPower = MyStats.BaseAttackPower * AttackFactor;
	float NewDefensePower = MyStats.BaseDefensePower * DefenseFactor;

	bool bChanged = (MyStats.AttackPower != NewAttackPower) || (MyStats.DefensePower != NewDefensePower);

	MyStats.AttackPower = NewAttackPower;
	MyStats.DefensePower = NewDefensePower;

	if (bChanged)
	{
		NotifyStatsChanged();
	}

	// --- 疲労段階が変わった時だけ、ActiveBuffsのバフアイコンを差し替える ---
	if (TargetFatigueBuffID != CurrentFatigueBuffID)
	{
		// 古い疲労バフアイコンを取り除く（予約済みStackID = FatigueBuffStackIDのものだけ）
		for (int32 i = ActiveBuffs.Num() - 1; i >= 0; --i)
		{
			if (ActiveBuffs[i].StackID == FatigueBuffStackID)
			{
				ActiveBuffs.RemoveAt(i);
			}
		}

		// 新しい段階に該当するバフがあれば、DT_Buffsから引いて追加する
		if (!TargetFatigueBuffID.IsNone() && BuffDataTable)
		{
			if (FBuffData* BuffRow = BuffDataTable->FindRow<FBuffData>(TargetFatigueBuffID, TEXT("FatigueBuffLookup")))
			{
				FActiveBuff NewBuff;
				NewBuff.BuffName = BuffRow->BuffName;
				NewBuff.BuffIcon = BuffRow->BuffIcon;
				NewBuff.ExpirationTime = -1.0f; // 期限なし（疲労度が下がるまで常時表示）の目印
				NewBuff.StackID = FatigueBuffStackID;
				ActiveBuffs.Add(NewBuff);
			}
		}

		CurrentFatigueBuffID = TargetFatigueBuffID;

		if (OnBuffListChangedDelegate.IsBound())
		{
			OnBuffListChangedDelegate.Broadcast();
		}
	}
}

void AMyProject1Character::AddFlag(FName FlagName)
{
	// フラグ名が空っぽではなく、まだ持っていない場合のみ追加する
	if (!FlagName.IsNone() && !MyStats.UnlockedFlags.Contains(FlagName))
	{
		MyStats.UnlockedFlags.Add(FlagName);
		OnFlagAdded.Broadcast(FlagName);
	}
}

bool AMyProject1Character::HasFlag(FName FlagName) const
{
	// 指定されたフラグを持っているか（含まれているか）を返す
	return MyStats.UnlockedFlags.Contains(FlagName);
}

void AMyProject1Character::RemoveFlag(FName FlagName)
{
	// リストから指定した名前を削除する（入れ物ごと消滅します！）
	// 実際に持っていた場合のみ通知する（OnFlagAddedと対称。VisibilityFlag等がその場で表示を更新できる）
	if (!FlagName.IsNone() && MyStats.UnlockedFlags.Contains(FlagName))
	{
		MyStats.UnlockedFlags.Remove(FlagName);
		OnFlagRemoved.Broadcast(FlagName);
	}
}

bool AMyProject1Character::TryUseSpecialAttack()
{
	if (JobRow.IsNull()) return false;

	FJobAttributes* JobData = JobRow.GetRow<FJobAttributes>(JobRow.RowName.ToString());
	if (!JobData || JobData->SpecialAttacks.Num() == 0) return false;

	double CurrentTime = GetWorld()->GetTimeSeconds();

	// リストに登録されている特殊技を上から順番にチェック
	for (int32 i = 0; i < JobData->SpecialAttacks.Num(); ++i)
	{
		const FSpecialAttackData& Skill = JobData->SpecialAttacks[i];
		if (!Skill.Montage) continue;

		// 1. クールダウンのチェック
		double LastUsed = SpecialAttackCooldowns.Contains(i) ? SpecialAttackCooldowns[i] : -9999.0;
		if (CurrentTime - LastUsed < Skill.Cooldown) continue;

		bool bCanUse = false;

		// 2. 条件の判定
		if (Skill.Condition == ESpecialCondition::HPBelowPercent)
		{
			float CurrentPercent = (MyStats.MaxHP > 0) ? (MyStats.HP / MyStats.MaxHP) * 100.0f : 0.0f;
			if (CurrentPercent <= Skill.ConditionValue)
			{
				bCanUse = true;
			}
		}
		else if (Skill.Condition == ESpecialCondition::AttackCount)
		{
			// 指定回数「以上」になったら発動
			if (ConsecutiveAttackCount >= FMath::RoundToInt(Skill.ConditionValue) && ConsecutiveAttackCount > 0)
			{
				bCanUse = true;
			}
		}

		// 3. 発動できる場合
		if (bCanUse)
		{
			CurrentExecutingSkillData = Skill;
			PerformSpecialAttack(Skill.Montage); // 発動！
			SpecialAttackCooldowns.Add(i, CurrentTime); // 使った時間を記録

			// もし回数条件だったら、発動後にカウントを0に戻す
			if (Skill.Condition == ESpecialCondition::AttackCount)
			{
				ConsecutiveAttackCount = 0;
			}

			// ログを確実にプレイヤーの画面に送る
			FString SpeakerName = MyStats.NPCName.IsEmpty() ? CharacterName : MyStats.NPCName;
			FString Msg = FString::Printf(TEXT("%s は 【%s】 の構え！"), *SpeakerName, *Skill.SkillName);

			if (IsPlayerControlled())
			{
				// 自分が使った場合は自分の画面に表示
				OnReceiveLogMessage(Msg, ELogMessageType::System);
			}
			else if (CurrentTarget)
			{
				// 敵が使った場合、狙われているプレイヤーの画面に送りつける！
				AMyProject1Character* TargetPlayer = Cast<AMyProject1Character>(CurrentTarget);
				if (TargetPlayer && TargetPlayer->IsPlayerControlled())
				{
					TargetPlayer->OnReceiveLogMessage(Msg, ELogMessageType::System);
				}
			}

			return true; // 1つ発動したら終了（複数同時には発動しない）
		}
	}
	return false;
}

void AMyProject1Character::UpdateBlink(float DeltaTime)
{
	if (!GetMesh()) return;

	// まばたきをしていない待機時間
	if (!bIsBlinking)
	{
		TimeUntilNextBlink -= DeltaTime;
		if (TimeUntilNextBlink <= 0.0f)
		{
			bIsBlinking = true;
			bIsClosingEyes = true; // 目を閉じ始める
		}
	}
	// まばたきのアニメーション中
	else
	{
		if (bIsClosingEyes)
		{
			// 目を滑らかに閉じる (0.0 -> 1.0)
			CurrentBlinkValue = FMath::FInterpConstantTo(CurrentBlinkValue, 1.0f, DeltaTime, BlinkSpeed);
			if (CurrentBlinkValue >= 1.0f)
			{
				bIsClosingEyes = false; // 完全に閉じたら開き始めるフラグに変える
			}
		}
		else
		{
			// 目を滑らかに開く (1.0 -> 0.0)
			CurrentBlinkValue = FMath::FInterpConstantTo(CurrentBlinkValue, 0.0f, DeltaTime, BlinkSpeed);
			if (CurrentBlinkValue <= 0.0f)
			{
				// 完全に開ききったらまばたき終了
				bIsBlinking = false;
				// 次のまばたきまでの時間を再設定
				TimeUntilNextBlink = FMath::RandRange(BlinkIntervalMin, BlinkIntervalMax);
			}
		}

		// 計算したモーフターゲットの値をメッシュに適用
		GetMesh()->SetMorphTarget(BlinkMorphName, CurrentBlinkValue);
	}
}

// ==========================================
// 装備システムの関数実装
// ==========================================

void AMyProject1Character::ApplyDefaultEquipment()
{
	if (!EquipmentDataTable) return;

	for (const FName& RowName : DefaultEquipmentRowNames)
	{
		if (RowName.IsNone()) continue;

		// すでに何か装備しているスロットは上書きしない（他経路で先に着せた分を尊重する）
		FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(RowName, TEXT("PlayerDefaultEquipment"));
		if (!EquipData) continue;
		if (CurrentEquippedItems.Contains(EquipData->TargetSlot)) continue;

		// EquipItem()末尾でRefreshEquipmentStats()も呼ばれるため、StatModifiersも通常どおり反映される
		EquipItem(RowName, *EquipData);
	}
}

void AMyProject1Character::EquipItem(FName ItemID, FEquipmentData EquipData)
{
	// ==========================================
	// 同じスロットに直接上書き装備された場合の古いテクスチャ装備のクリア
	// ==========================================
	if (CurrentEquippedItems.Contains(EquipData.TargetSlot))
	{
		FName OldItemID = CurrentEquippedItems[EquipData.TargetSlot];
		if (!OldItemID.IsNone() && EquipmentDataTable)
		{
			FEquipmentData* OldEquipData = EquipmentDataTable->FindRow<FEquipmentData>(OldItemID, TEXT("OldEquipmentTextureCleanup"));
			// もし古い装備がテクスチャオーバーレイを持っていたら、先に透明化する
			if (OldEquipData && !OldEquipData->OverlayTexture.IsNull())
			{
				if (USkeletalMeshComponent* BodyMesh = GetMesh())
				{
					UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(BodyMesh->GetMaterial(0));
					if (MID)
					{
						MID->SetScalarParameterValue(OldEquipData->OpacityParamName, 0.0f);
					}
				}
			}
		}
	}

	    // ==========================================
		// 薄地装備（テクスチャ方式）が設定されていればUIへ通知
		// ※実際のマテリアルへの反映は、この関数の末尾で呼ぶ SkinOverlayComp->RefreshBodyMaterials() が
		//   CurrentEquippedItems を見て一括で行う（旧：MIDへ直接SetParameterしていたコードは削除）
		// ==========================================
		if (!EquipData.OverlayTexture.IsNull())
		{
			if (OnSkinOverlayUIChangedDelegate.IsBound())
			{
				OnSkinOverlayUIChangedDelegate.Broadcast();
			}
		}

	switch (EquipData.TargetSlot)
	{

	case EEquipmentSlot::Hair:
		if (HairMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			HairMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Head:
		if (HeadMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			HeadMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Torso:
		if (TorsoMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			TorsoMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Waist:
		if (WaistMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			WaistMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::InnerUpper:
		if (InnerUpperMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			InnerUpperMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::InnerLower:
		if (InnerLowerMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			InnerLowerMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Hands:
		if (HandsMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			HandsMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Legs:
		if (LegsMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			LegsMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Feet:
		if (FeetMeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			FeetMeshComp->SetSkeletalMesh(LoadedMesh);
		}
		ApplyShoesOffset(true, EquipData.HeightOffset, EquipData.AnkleRotationOffset, EquipData.ToeRotationOffset);
		break;

	case EEquipmentSlot::Neck:
		if (NeckSkeletalMeshComp)
		{
			USkeletalMesh* LoadedSkelMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			NeckSkeletalMeshComp->SetSkeletalMesh(LoadedSkelMesh);
		}
		// ▼ スタティックメッシュ（ペンダントなど）が設定されていれば読み込む
		if (NeckMeshComp)
		{
			UStaticMesh* LoadedStatMesh = EquipData.EquipStaticMesh.IsNull() ? nullptr : EquipData.EquipStaticMesh.LoadSynchronous();
			NeckMeshComp->SetStaticMesh(LoadedStatMesh);
		}
		break;

	case EEquipmentSlot::Wrist:
		// ▼ スケルタルメッシュ（カフスなど）が設定されていれば読み込む、なければ空にする
		if (WristSkeletalMeshComp)
		{
			USkeletalMesh* LoadedSkelMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			WristSkeletalMeshComp->SetSkeletalMesh(LoadedSkelMesh);
		}

		// ▼ スタティックメッシュ（固い腕輪など）が設定されていれば読み込む、なければ空にする
		if (WristMeshComp)
		{
			UStaticMesh* LoadedStatMesh = EquipData.EquipStaticMesh.IsNull() ? nullptr : EquipData.EquipStaticMesh.LoadSynchronous();
			WristMeshComp->SetStaticMesh(LoadedStatMesh);
		}
		break;

	case EEquipmentSlot::Ankle:
		if (AnkleSkeletalMeshComp)
		{
			USkeletalMesh* LoadedSkelMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			AnkleSkeletalMeshComp->SetSkeletalMesh(LoadedSkelMesh);
		}
		// ▼ スタティックメッシュ（アンクレットなど）が設定されていれば読み込む
		if (AnkleMeshComp)
		{
			UStaticMesh* LoadedStatMesh = EquipData.EquipStaticMesh.IsNull() ? nullptr : EquipData.EquipStaticMesh.LoadSynchronous();
			AnkleMeshComp->SetStaticMesh(LoadedStatMesh);
		}
		break;

	case EEquipmentSlot::Extra1:
		if (Extra1MeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			Extra1MeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Extra2:
		if (Extra2MeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			Extra2MeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Extra3:
		if (Extra3MeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			Extra3MeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Extra4:
		if (Extra4MeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			Extra4MeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	case EEquipmentSlot::Extra5:
		if (Extra5MeshComp)
		{
			USkeletalMesh* LoadedMesh = EquipData.EquipSkeletalMesh.IsNull() ? nullptr : EquipData.EquipSkeletalMesh.LoadSynchronous();
			Extra5MeshComp->SetSkeletalMesh(LoadedMesh);
		}
		break;

	default:
		break;
	}

	// 鎖の基準となるコンポーネントをスロットに応じて決定
	USceneComponent* CurrentActiveComp = nullptr;

	if (EquipData.TargetSlot == EEquipmentSlot::Feet)
	{
		CurrentActiveComp = FeetMeshComp;
	}
	else if (EquipData.TargetSlot == EEquipmentSlot::Neck)
	{
		CurrentActiveComp = NeckMeshComp;
	}
	else if (EquipData.TargetSlot == EEquipmentSlot::Hands)
	{
		// 手袋・小手などのスケルタルメッシュを対象にする
		CurrentActiveComp = HandsMeshComp;
	}
	else if (EquipData.TargetSlot == EEquipmentSlot::Wrist)
	{
		// 腕輪（手枷）はStaticMeshとSkeletalMeshの両方があるため、
		// StaticMeshが設定されていればそれを使い、なければSkeletalMeshを対象にする
		if (WristMeshComp && WristMeshComp->GetStaticMesh())
		{
			CurrentActiveComp = WristMeshComp;
		}
		else
		{
			CurrentActiveComp = WristSkeletalMeshComp;
		}
	}
	else if (EquipData.TargetSlot == EEquipmentSlot::Ankle) // ★ここから追加！
	{
		// 足輪（足枷）はStaticMeshとSkeletalMeshの両方があるため、手首と同様に判定
		if (AnkleMeshComp && AnkleMeshComp->GetStaticMesh())
		{
			CurrentActiveComp = AnkleMeshComp;
		}
		else
		{
			CurrentActiveComp = AnkleSkeletalMeshComp;
		}
	} 

	// 鎖の設定があれば構築を実行する
	if (EquipData.CableSettings.AttachType != ECableAttachType::None)
	{
		// ★引数に現在の TargetSlot を追加
		SetupCableSystem(EquipData.CableSettings, EquipData.TargetSlot, CurrentActiveComp);
	}

	// 装備状態の記憶を更新（ここで ItemID が使われます）
	CurrentEquippedItems.Add(EquipData.TargetSlot, ItemID);

	// 薄地装備（テクスチャ方式）を含めたボディオーバーレイをCanvas合成システムで再構築
	if (SkinOverlayComp)
	{
		SkinOverlayComp->RefreshBodyMaterials();
	}

	// 胴・腰装備の非表示設定に応じてインナーの表示を更新（透け防止）
	RefreshInnerVisibility();

	RefreshEquipmentStats();
	RefreshMovementRestriction();
}

void AMyProject1Character::UnequipItem(EEquipmentSlot TargetSlot)
{
	bool bHasCable = false;
	FName EquippedItemID = GetEquippedItemID(TargetSlot);
	if (!EquippedItemID.IsNone() && EquipmentDataTable)
	{
		FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(EquippedItemID, TEXT("UnequipCableCheck"));
		if (EquipData && EquipData->CableSettings.AttachType != ECableAttachType::None)
		{
			bHasCable = true;
		}

		    // ==========================================
			// 薄地装備（テクスチャ方式）の解除をUIへ通知
			// ※実際のマテリアルへの反映は、この関数の末尾で呼ぶ SkinOverlayComp->RefreshBodyMaterials() が
			//   CurrentEquippedItems を見て一括で行う（旧：MIDへ直接SetParameterしていたコードは削除）
			// ==========================================
			if (!EquipData->OverlayTexture.IsNull())
			{
				if (OnSkinOverlayUIChangedDelegate.IsBound())
				{
					OnSkinOverlayUIChangedDelegate.Broadcast();
				}
			}


	}

	switch (TargetSlot)
	{
	case EEquipmentSlot::Hair:
		if (HairMeshComp) HairMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Head:
		if (HeadMeshComp) HeadMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Torso:
		if (TorsoMeshComp) TorsoMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Waist:
		if (WaistMeshComp) WaistMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::InnerUpper:
		if (InnerUpperMeshComp) InnerUpperMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::InnerLower:
		if (InnerLowerMeshComp) InnerLowerMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Hands:
		if (HandsMeshComp) HandsMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Legs:
		if (LegsMeshComp) LegsMeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Feet:
		if (FeetMeshComp) FeetMeshComp->SetSkeletalMesh(nullptr);
		ApplyShoesOffset(false);
		break;

	case EEquipmentSlot::Neck:
		if (NeckSkeletalMeshComp) NeckSkeletalMeshComp->SetSkeletalMesh(nullptr);
		if (NeckMeshComp) NeckMeshComp->SetStaticMesh(nullptr);
		break;

	case EEquipmentSlot::Wrist:
		if (WristSkeletalMeshComp) WristSkeletalMeshComp->SetSkeletalMesh(nullptr);
		if (WristMeshComp) WristMeshComp->SetStaticMesh(nullptr);
		break;

	case EEquipmentSlot::Ankle:
		if (AnkleSkeletalMeshComp) AnkleSkeletalMeshComp->SetSkeletalMesh(nullptr);
		if (AnkleMeshComp) AnkleMeshComp->SetStaticMesh(nullptr);
		break;

	case EEquipmentSlot::Extra1:
		if (Extra1MeshComp) Extra1MeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Extra2:
		if (Extra2MeshComp) Extra2MeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Extra3:
		if (Extra3MeshComp) Extra3MeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Extra4:
		if (Extra4MeshComp) Extra4MeshComp->SetSkeletalMesh(nullptr);
		break;

	case EEquipmentSlot::Extra5:
		if (Extra5MeshComp) Extra5MeshComp->SetSkeletalMesh(nullptr);
		break;

	default:
		break;
	}

	if (bHasCable)
	{
		ClearCableSystem(TargetSlot);
	}

	// 記憶している装備状態から削除
	CurrentEquippedItems.Remove(TargetSlot);

	// 薄地装備（テクスチャ方式）を含めたボディオーバーレイをCanvas合成システムで再構築
	if (SkinOverlayComp)
	{
		SkinOverlayComp->RefreshBodyMaterials();
	}

	// 胴・腰装備の非表示設定に応じてインナーの表示を更新（外したらインナーを再表示）
	RefreshInnerVisibility();

	RefreshEquipmentStats();
	RefreshMovementRestriction();
}

bool AMyProject1Character::IsSlotLocked(EEquipmentSlot TargetSlot)
{
	const FName EquippedItemID = GetEquippedItemID(TargetSlot);
	if (EquippedItemID.IsNone() || !EquipmentDataTable) return false;

	const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(EquippedItemID, TEXT("IsSlotLocked"));
	return EquipData && EquipData->bCannotUnequipManually;
}

bool AMyProject1Character::TryUnequipItem(EEquipmentSlot TargetSlot)
{
	if (IsSlotLocked(TargetSlot))
	{
		// 表示名は DevMemo（開発用メモ＝アイテム名）を使う。無ければ行名。
		const FName EquippedItemID = GetEquippedItemID(TargetSlot);
		FString DisplayName = EquippedItemID.ToString();
		if (EquipmentDataTable)
		{
			if (const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(EquippedItemID, TEXT("TryUnequipItem")))
			{
				if (!EquipData->DevMemo.IsEmpty()) DisplayName = EquipData->DevMemo;
			}
		}
		OnReceiveLogMessage(FString::Printf(TEXT("%s は自力では外せない。"), *DisplayName), ELogMessageType::System);
		return false;
	}

	UnequipItem(TargetSlot);
	return true;
}

bool AMyProject1Character::ForceRemoveLockedEquipment(EEquipmentSlot TargetSlot, bool bReturnToInventory)
{
	// ロック装備でないスロットには作用させない（誤爆防止）
	if (!IsSlotLocked(TargetSlot)) return false;

	const FName RemovedItemID = GetEquippedItemID(TargetSlot);

	UnequipItem(TargetSlot);

	// 鍵・ショップ解除はインベントリに戻す。破壊解除は戻さない（＝消滅）。
	if (bReturnToInventory && !RemovedItemID.IsNone() && InventoryComp)
	{
		InventoryComp->AddItem(RemovedItemID, 1);
	}

	return true;
}

TArray<EEquipmentSlot> AMyProject1Character::GetLockedEquipmentSlots()
{
	TArray<EEquipmentSlot> Result;
	if (!EquipmentDataTable) return Result;

	for (const TPair<EEquipmentSlot, FName>& Pair : CurrentEquippedItems)
	{
		if (Pair.Value.IsNone()) continue;

		const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(Pair.Value, TEXT("GetLockedEquipmentSlots"));
		if (EquipData && EquipData->bCannotUnequipManually)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool AMyProject1Character::TryShopUnlockRestraint(EEquipmentSlot TargetSlot, int32 ShopLevel)
{
	if (!IsSlotLocked(TargetSlot) || !EquipmentDataTable) return false;

	const FName EquippedItemID = GetEquippedItemID(TargetSlot);
	const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(EquippedItemID, TEXT("TryShopUnlockRestraint"));
	if (!EquipData) return false;

	FString DisplayName = EquipData->DevMemo.IsEmpty() ? EquippedItemID.ToString() : EquipData->DevMemo;

	// 職人レベル判定
	if (ShopLevel < EquipData->UnlockLevel)
	{
		OnReceiveLogMessage(FString::Printf(TEXT("ここの職人では %s は外せないようだ。"), *DisplayName), ELogMessageType::System);
		return false;
	}

	// ギル支払い（TrySpendGilは足りなければ何もせずfalse）
	if (!InventoryComp || !InventoryComp->TrySpendGil(EquipData->UnlockPrice))
	{
		OnReceiveLogMessage(FString::Printf(TEXT("解除費用（%dギル）が足りない。"), EquipData->UnlockPrice), ELogMessageType::System);
		return false;
	}

	// 解除（インベントリに戻す）
	ForceRemoveLockedEquipment(TargetSlot, /*bReturnToInventory=*/true);
	OnReceiveLogMessage(FString::Printf(TEXT("%s を外してもらった。"), *DisplayName), ELogMessageType::System);
	return true;
}

bool AMyProject1Character::IsPiercingSlot(EEquipmentSlot Slot)
{
	return Slot == EEquipmentSlot::Extra1 || Slot == EEquipmentSlot::Extra2
		|| Slot == EEquipmentSlot::Extra3 || Slot == EEquipmentSlot::Extra4
		|| Slot == EEquipmentSlot::Extra5;
}

TArray<EEquipmentSlot> AMyProject1Character::GetEquippedCursedPiercingSlots()
{
	TArray<EEquipmentSlot> Result;
	if (!EquipmentDataTable) return Result;

	for (const TPair<EEquipmentSlot, FName>& Pair : CurrentEquippedItems)
	{
		if (Pair.Value.IsNone() || !IsPiercingSlot(Pair.Key)) continue;

		const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(Pair.Value, TEXT("GetEquippedCursedPiercingSlots"));
		if (EquipData && EquipData->bCannotUnequipManually)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> AMyProject1Character::GetAllPiercingEquipmentRowNames()
{
	TArray<FName> Result;
	if (!EquipmentDataTable) return Result;

	for (const FName& RowName : EquipmentDataTable->GetRowNames())
	{
		const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(RowName, TEXT("GetAllPiercingEquipmentRowNames"));
		if (EquipData && IsPiercingSlot(EquipData->TargetSlot))
		{
			Result.Add(RowName);
		}
	}
	return Result;
}

bool AMyProject1Character::TryDoctorRemovePiercing(FName EquipRowName, int32 ShopLevel)
{
	if (EquipRowName.IsNone() || !EquipmentDataTable) return false;

	// 装備中の呪われピアス（Extra1〜5 かつ bCannotUnequipManually）から、行名が一致するスロットを探す
	EEquipmentSlot TargetSlot = EEquipmentSlot::Max;
	for (const TPair<EEquipmentSlot, FName>& Pair : CurrentEquippedItems)
	{
		if (Pair.Value == EquipRowName && IsPiercingSlot(Pair.Key) && IsSlotLocked(Pair.Key))
		{
			TargetSlot = Pair.Key;
			break;
		}
	}
	if (TargetSlot == EEquipmentSlot::Max) return false;

	const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(EquipRowName, TEXT("TryDoctorRemovePiercing"));
	if (!EquipData) return false;

	const FString DisplayName = EquipData->DevMemo.IsEmpty() ? EquipRowName.ToString() : EquipData->DevMemo;

	if (ShopLevel < EquipData->UnlockLevel)
	{
		OnReceiveLogMessage(FString::Printf(TEXT("この医者では %s は外せないようだ。"), *DisplayName), ELogMessageType::System);
		return false;
	}

	if (!InventoryComp || !InventoryComp->TrySpendGil(EquipData->UnlockPrice))
	{
		OnReceiveLogMessage(FString::Printf(TEXT("施術費（%dギル）が足りない。"), EquipData->UnlockPrice), ELogMessageType::System);
		return false;
	}

	// 呪われピアスの除去＝消滅（インベントリには戻さない）
	ForceRemoveLockedEquipment(TargetSlot, /*bReturnToInventory=*/false);
	OnReceiveLogMessage(FString::Printf(TEXT("%s を外してもらった。"), *DisplayName), ELogMessageType::System);

	// 既存の施術ショップUIはこのデリゲートでリストを更新する
	if (OnSkinOverlayUIChangedDelegate.IsBound())
	{
		OnSkinOverlayUIChangedDelegate.Broadcast();
	}
	return true;
}

bool AMyProject1Character::TryDoctorAddPiercing(FName EquipRowName, int32 PriceMarkup)
{
	if (EquipRowName.IsNone() || !EquipmentDataTable) return false;

	FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(EquipRowName, TEXT("TryDoctorAddPiercing"));
	if (!EquipData || !IsPiercingSlot(EquipData->TargetSlot)) return false;

	const FString DisplayName = EquipData->DevMemo.IsEmpty() ? EquipRowName.ToString() : EquipData->DevMemo;

	// 同じスロットに既にピアスがあるなら不可
	if (CurrentEquippedItems.Contains(EquipData->TargetSlot))
	{
		OnReceiveLogMessage(TEXT("そこには既にピアスが入っている。"), ELogMessageType::System);
		return false;
	}

	// 価格 = DT_Items の Price ＋ 施術マークアップ（店ごとに設定）
	int32 BasePrice = 0;
	if (InventoryComp && InventoryComp->ItemDataTable)
	{
		if (const FItemData* ItemData = InventoryComp->ItemDataTable->FindRow<FItemData>(EquipRowName, TEXT("TryDoctorAddPiercing_Price")))
		{
			BasePrice = ItemData->Price;
		}
	}
	const int32 FinalPrice = FMath::Max(0, BasePrice + PriceMarkup);

	if (!InventoryComp || !InventoryComp->TrySpendGil(FinalPrice))
	{
		OnReceiveLogMessage(FString::Printf(TEXT("施術費（%dギル）が足りない。"), FinalPrice), ELogMessageType::System);
		return false;
	}

	// 店で買ってその場で装着（インベントリ経由なし）
	EquipItem(EquipRowName, *EquipData);
	OnReceiveLogMessage(FString::Printf(TEXT("%s を装着した。%dギルを支払った。"), *DisplayName, FinalPrice), ELogMessageType::System);

	if (OnSkinOverlayUIChangedDelegate.IsBound())
	{
		OnSkinOverlayUIChangedDelegate.Broadcast();
	}
	return true;
}

TArray<FOverlayShopItemInfo> AMyProject1Character::BuildPiercingShopList() const
{
	TArray<FOverlayShopItemInfo> OutList;
	if (!EquipmentDataTable) return OutList;

	// 店の技術レベルと施術マークアップ（NPCがいなければ制限なし・マークアップ既定値）
	int32 ShopLevel = 99;
	int32 AddMarkup = 10000;
	if (ActiveShopNPC)
	{
		ShopLevel = ActiveShopNPC->ShopLevel;
		AddMarkup = ActiveShopNPC->PiercingAddMarkup;
	}

	const UDataTable* ItemDT = (InventoryComp ? InventoryComp->ItemDataTable : nullptr);

	// --- 装備中の「呪われピアス」＝除去対象（bIsOwned=true） ---
	for (const TPair<EEquipmentSlot, FName>& Pair : CurrentEquippedItems)
	{
		if (Pair.Value.IsNone() || !IsPiercingSlot(Pair.Key)) continue;

		const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(Pair.Value, TEXT("BuildPiercingShopList_Remove"));
		if (!EquipData || !EquipData->bCannotUnequipManually) continue;

		FOverlayShopItemInfo Info;
		Info.RowName = Pair.Value;
		Info.bIsOwned = true;
		Info.RemovePrice = EquipData->UnlockPrice;
		Info.BuyPrice = 0;
		if (ItemDT)
		{
			if (const FItemData* ItemData = ItemDT->FindRow<FItemData>(Pair.Value, TEXT("BuildPiercingShopList_RemoveName")))
			{
				Info.DisplayName = ItemData->Name;
				Info.Description = ItemData->Description;
			}
		}
		if (Info.DisplayName.IsEmpty()) Info.DisplayName = EquipData->DevMemo.IsEmpty() ? Pair.Value.ToString() : EquipData->DevMemo;
		OutList.Add(Info);
	}

	// --- 未装備のピアス装備＝カタログ（bIsOwned=false） ---
	for (const FName& RowName : EquipmentDataTable->GetRowNames())
	{
		const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(RowName, TEXT("BuildPiercingShopList_Add"));
		if (!EquipData || !IsPiercingSlot(EquipData->TargetSlot)) continue;

		// そのスロットが既に埋まっているカタログ品は出さない（買っても装着できないため）
		if (CurrentEquippedItems.Contains(EquipData->TargetSlot)) continue;

		// 店の技術レベルが足りないものは並べない（他の施術ショップと同じ挙動）
		if (EquipData->UnlockLevel > ShopLevel) continue;

		int32 BasePrice = 0;
		FString ItemName;
		FText ItemDesc = FText::GetEmpty();
		if (ItemDT)
		{
			if (const FItemData* ItemData = ItemDT->FindRow<FItemData>(RowName, TEXT("BuildPiercingShopList_AddName")))
			{
				BasePrice = ItemData->Price;
				ItemName = ItemData->Name;
				ItemDesc = ItemData->Description;
			}
		}

		FOverlayShopItemInfo Info;
		Info.RowName = RowName;
		Info.bIsOwned = false;
		Info.BuyPrice = FMath::Max(0, BasePrice + AddMarkup);
		Info.RemovePrice = 0;
		Info.DisplayName = ItemName.IsEmpty() ? (EquipData->DevMemo.IsEmpty() ? RowName.ToString() : EquipData->DevMemo) : ItemName;
		Info.Description = ItemDesc;
		OutList.Add(Info);
	}

	return OutList;
}

TArray<FOverlayShopItemInfo> AMyProject1Character::BuildRestraintShopList() const
{
	TArray<FOverlayShopItemInfo> OutList;
	if (!EquipmentDataTable) return OutList;

	const UDataTable* ItemDT = (InventoryComp ? InventoryComp->ItemDataTable : nullptr);

	// 現在装備中の「自力では外せないロック装備」＝解除対象。
	// ピアス枠（Extra1〜5）はピアス医者ショップの専任のため、ここでは除外する。
	for (const TPair<EEquipmentSlot, FName>& Pair : CurrentEquippedItems)
	{
		if (Pair.Value.IsNone() || IsPiercingSlot(Pair.Key)) continue;

		const FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(Pair.Value, TEXT("BuildRestraintShopList_Remove"));
		if (!EquipData || !EquipData->bCannotUnequipManually) continue;

		FOverlayShopItemInfo Info;
		Info.RowName = Pair.Value;
		Info.bIsOwned = true;
		Info.RemovePrice = EquipData->UnlockPrice;
		Info.BuyPrice = 0;
		if (ItemDT)
		{
			if (const FItemData* ItemData = ItemDT->FindRow<FItemData>(Pair.Value, TEXT("BuildRestraintShopList_RemoveName")))
			{
				Info.DisplayName = ItemData->Name;
				Info.Description = ItemData->Description;
			}
		}
		if (Info.DisplayName.IsEmpty()) Info.DisplayName = EquipData->DevMemo.IsEmpty() ? Pair.Value.ToString() : EquipData->DevMemo;
		OutList.Add(Info);
	}

	return OutList;
}

void AMyProject1Character::RefreshMovementRestriction()
{
	// 速度：複数の拘束具を同時装備している場合、最も制限の強い（＝遅い歩き優先の）
	// 装備1つを「勝者」として選び、その速度プリセットだけを採用する（負けた側の速度は無視）。
	// ABP差し替え：速度とは独立に判定する。どれか1つでも「ブレンドスペース変更」に
	// チェックが入っていればON（OR条件）。速度側の勝敗とは連動させない。
	const FEquipmentData* WinningRestraintRow = nullptr;
	bool bNewAnimOverride = false;

	if (EquipmentDataTable)
	{
		for (const TPair<EEquipmentSlot, FName>& Pair : CurrentEquippedItems)
		{
			if (Pair.Value.IsNone())
			{
				continue;
			}

			if (const FEquipmentData* Row = EquipmentDataTable->FindRow<FEquipmentData>(Pair.Value, TEXT("RefreshMovementRestriction")))
			{
				if (!Row->bRestrictsMovement)
				{
					continue;
				}

				const bool bRowIsSlowWalk = (Row->SpeedPreset == ERestrainedSpeedPreset::SlowWalk);
				const bool bCurrentWinnerIsSlowWalk = WinningRestraintRow && (WinningRestraintRow->SpeedPreset == ERestrainedSpeedPreset::SlowWalk);

				// まだ勝者がいない、または今見ている装備の方が制限が強い（遅い歩き）場合に勝者を更新する
				if (!WinningRestraintRow || (bRowIsSlowWalk && !bCurrentWinnerIsSlowWalk))
				{
					WinningRestraintRow = Row;
				}

				if (Row->bOverridesLocomotionAnim)
				{
					bNewAnimOverride = true;
				}
			}
		}
	}

	const bool bNewRestricted = (WinningRestraintRow != nullptr);

	bIsMovementRestricted = bNewRestricted;

	if (bNewRestricted)
	{
		if (UMyProject1GameInstance* GI = GetGameInstance<UMyProject1GameInstance>())
		{
			RestrainedSpeedCap = GI->GetRestrainedSpeed(WinningRestraintRow->SpeedPreset);
		}
	}
	else
	{
		RestrainedSpeedCap = 0.0f;
	}

	// ABPの差し替え/復元は状態が変化した時だけ行う（毎回SetAnimInstanceClassし直すとAnim側の状態がリセットされるため）
	if (bNewAnimOverride != bUsesRestrainedAnimBlueprint)
	{
		bUsesRestrainedAnimBlueprint = bNewAnimOverride;
		ApplyRestrainedAnimBlueprint();
	}
}

void AMyProject1Character::ApplyRestrainedAnimBlueprint()
{
	if (!GetMesh())
	{
		return;
	}

	if (bUsesRestrainedAnimBlueprint)
	{
		UMyProject1GameInstance* GI = GetGameInstance<UMyProject1GameInstance>();
		TSubclassOf<UAnimInstance> RestrainedClass = GI ? GI->RestrainedAnimBlueprintClass.LoadSynchronous() : nullptr;

		if (!RestrainedClass)
		{
			return;
		}

		// 元のABPを記憶しておく（解除時に復元するため。既に記憶済みなら上書きしない）
		if (!DefaultAnimBlueprintClass)
		{
			if (UAnimInstance* CurrentAnimInst = GetMesh()->GetAnimInstance())
			{
				DefaultAnimBlueprintClass = CurrentAnimInst->GetClass();
			}
		}

		GetMesh()->SetAnimInstanceClass(RestrainedClass);
	}
	else if (DefaultAnimBlueprintClass)
	{
		GetMesh()->SetAnimInstanceClass(DefaultAnimBlueprintClass);
		DefaultAnimBlueprintClass = nullptr;
	}

	// ABPを差し替えると新しいAnimInstanceインスタンスに変わるため、モンタージュ終了イベントの監視を張り直す
	if (UAnimInstance* NewAnimInst = GetMesh()->GetAnimInstance())
	{
		if (!NewAnimInst->OnMontageEnded.IsAlreadyBound(this, &AMyProject1Character::OnMontageEnded))
		{
			NewAnimInst->OnMontageEnded.AddDynamic(this, &AMyProject1Character::OnMontageEnded);
		}
	}
}

void AMyProject1Character::RefreshInnerVisibility()
{
	bool bHideUpper = false;
	bool bHideLower = false;

	if (EquipmentDataTable)
	{
		if (const FName* TorsoItemID = CurrentEquippedItems.Find(EEquipmentSlot::Torso))
		{
			if (!TorsoItemID->IsNone())
			{
				if (FEquipmentData* TorsoData = EquipmentDataTable->FindRow<FEquipmentData>(*TorsoItemID, TEXT("RefreshInnerVisibility_Torso")))
				{
					bHideUpper |= TorsoData->bHideInnerUpper;
					bHideLower |= TorsoData->bHideInnerLower;
				}
			}
		}

		if (const FName* WaistItemID = CurrentEquippedItems.Find(EEquipmentSlot::Waist))
		{
			if (!WaistItemID->IsNone())
			{
				if (FEquipmentData* WaistData = EquipmentDataTable->FindRow<FEquipmentData>(*WaistItemID, TEXT("RefreshInnerVisibility_Waist")))
				{
					bHideLower |= WaistData->bHideInnerLower;
				}
			}
		}

		if (const FName* LegsItemID = CurrentEquippedItems.Find(EEquipmentSlot::Legs))
		{
			if (!LegsItemID->IsNone())
			{
				if (FEquipmentData* LegsData = EquipmentDataTable->FindRow<FEquipmentData>(*LegsItemID, TEXT("RefreshInnerVisibility_Legs")))
				{
					bHideLower |= LegsData->bHideInnerLower;
				}
			}
		}
	}

	if (InnerUpperMeshComp)
	{
		InnerUpperMeshComp->SetVisibility(!bHideUpper);
	}

	if (InnerLowerMeshComp)
	{
		InnerLowerMeshComp->SetVisibility(!bHideLower);
	}
}


void AMyProject1Character::RefreshEquipmentStats()
{
	if (JobRow.IsNull()) return;
	FJobAttributes* JobData = JobRow.GetRow<FJobAttributes>(JobRow.RowName.ToString());
	if (!JobData) return;

	// 1. ジョブとレベルから「基礎ステータス」を再計算
	float BaseMaxHP = JobData->BaseHP + ((MyStats.Level - 1) * 15.0f);
	float BaseSTR = JobData->BaseSTR + ((MyStats.Level - 1) * 2.0f);
	float BaseVIT = JobData->BaseVIT + ((MyStats.Level - 1) * 2.0f);
	float BaseDEX = JobData->BaseDEX + ((MyStats.Level - 1) * 2.0f);
	float BaseAGI = JobData->BaseAGI + ((MyStats.Level - 1) * 2.0f);

	// 2. 装備品のボーナスを合計（DT_Equipmentsの配列エレメントを自由に集計する仕組み）
	TMap<ETargetStat, float> StatBonuses;
	TMap<FName, float> NewExtraStatBonuses;

	// NPC（AQuestNPCBase）はShouldApplyEquipmentStatBonuses()をfalseに上書きしており、
	// ここをスキップすることでEquipItem()による見た目の変化のみを許可し、MyStatsは変化させない
	if (ShouldApplyEquipmentStatBonuses() && EquipmentDataTable)
	{
		// 現在装備中のアイテムをループして合計値を出す
		for (const auto& Pair : CurrentEquippedItems)
		{
			FName ItemID = Pair.Value;
			if (ItemID.IsNone()) continue;

			FEquipmentData* EquipData = EquipmentDataTable->FindRow<FEquipmentData>(ItemID, TEXT("EquipmentStats"));
			if (!EquipData) continue;

			for (const FEquipmentStatModifier& Modifier : EquipData->StatModifiers)
			{
				if (Modifier.TargetStat == ETargetStat::CustomExtraStat)
				{
					if (!Modifier.ExtraStatName.IsNone())
					{
						NewExtraStatBonuses.FindOrAdd(Modifier.ExtraStatName) += Modifier.Amount;
					}
				}
				else
				{
					StatBonuses.FindOrAdd(Modifier.TargetStat) += Modifier.Amount;
				}
			}
		}
	}

	// 2-b. 病気・ケガ／タトゥー／ピアス（SkinOverlayComponentの箱）のボーナスも同じ仕組みで合算する
	// ※NPCはShouldApplyEquipmentStatBonuses()がfalseのため、タトゥー等を付けても見た目だけでMyStatsは変化しない
	TMap<FName, float> NewSkinOverlayExtraStatBonuses;
	if (ShouldApplyEquipmentStatBonuses() && SkinOverlayComp)
	{
		for (const FEquipmentStatModifier& Modifier : SkinOverlayComp->GetActiveStatModifiers())
		{
			if (Modifier.TargetStat == ETargetStat::CustomExtraStat)
			{
				if (!Modifier.ExtraStatName.IsNone())
				{
					NewSkinOverlayExtraStatBonuses.FindOrAdd(Modifier.ExtraStatName) += Modifier.Amount;
				}
			}
			else
			{
				StatBonuses.FindOrAdd(Modifier.TargetStat) += Modifier.Amount;
			}
		}
	}

	// 2-c. 月齢サイクルの現在フェーズのボーナスも同じ仕組みで合算する（フェーズが変わって再計算されると自動的に元に戻る）
	TMap<FName, float> NewCyclePhaseExtraStatBonuses;
	if (ShouldApplyEquipmentStatBonuses())
	{
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
		{
			for (const FCyclePhaseSettings& Rule : GameInst->CyclePhaseRules)
			{
				if (CurrentCycleDay >= Rule.MinDay && CurrentCycleDay <= Rule.MaxDay)
				{
					for (const FEquipmentStatModifier& Modifier : Rule.StatModifiers)
					{
						if (Modifier.TargetStat == ETargetStat::CustomExtraStat)
						{
							if (!Modifier.ExtraStatName.IsNone())
							{
								NewCyclePhaseExtraStatBonuses.FindOrAdd(Modifier.ExtraStatName) += Modifier.Amount;
							}
						}
						else
						{
							StatBonuses.FindOrAdd(Modifier.TargetStat) += Modifier.Amount;
						}
					}
					break; // 一致するルールが見つかったのでループを抜ける（UpdateCycleStateの判定と同じ規則）
				}
			}
		}
	}

	auto GetBonus = [&StatBonuses](ETargetStat Stat) -> float
	{
		const float* Found = StatBonuses.Find(Stat);
		return Found ? *Found : 0.0f;
	};

	// 2-c. ExtraStats（ex_stats）はアイテム消費などでも永続的に増減する値のため、
	//      ここでは「前回の装備が加算していた分」との差分だけを反映して二重加算を防ぐ
	for (const auto& NewPair : NewExtraStatBonuses)
	{
		const float OldBonus = EquipmentExtraStatBonuses.FindRef(NewPair.Key);
		const float Delta = NewPair.Value - OldBonus;
		if (Delta != 0.0f)
		{
			MyStats.ExtraStats.FindOrAdd(NewPair.Key) += Delta;
		}
	}
	for (const auto& OldPair : EquipmentExtraStatBonuses)
	{
		if (!NewExtraStatBonuses.Contains(OldPair.Key))
		{
			MyStats.ExtraStats.FindOrAdd(OldPair.Key) -= OldPair.Value;
		}
	}
	EquipmentExtraStatBonuses = NewExtraStatBonuses;

	// 2-d. 病気・ケガ／タトゥー／ピアス分のExtraStatsも、装備分とは別の記録との差分だけを反映する
	for (const auto& NewPair : NewSkinOverlayExtraStatBonuses)
	{
		const float OldBonus = SkinOverlayExtraStatBonuses.FindRef(NewPair.Key);
		const float Delta = NewPair.Value - OldBonus;
		if (Delta != 0.0f)
		{
			MyStats.ExtraStats.FindOrAdd(NewPair.Key) += Delta;
		}
	}
	for (const auto& OldPair : SkinOverlayExtraStatBonuses)
	{
		if (!NewSkinOverlayExtraStatBonuses.Contains(OldPair.Key))
		{
			MyStats.ExtraStats.FindOrAdd(OldPair.Key) -= OldPair.Value;
		}
	}
	SkinOverlayExtraStatBonuses = NewSkinOverlayExtraStatBonuses;

	// 2-e. 月齢サイクルのフェーズ分のExtraStatsも、他の記録とは別に差分だけを反映する
	for (const auto& NewPair : NewCyclePhaseExtraStatBonuses)
	{
		const float OldBonus = CyclePhaseExtraStatBonuses.FindRef(NewPair.Key);
		const float Delta = NewPair.Value - OldBonus;
		if (Delta != 0.0f)
		{
			MyStats.ExtraStats.FindOrAdd(NewPair.Key) += Delta;
		}
	}
	for (const auto& OldPair : CyclePhaseExtraStatBonuses)
	{
		if (!NewCyclePhaseExtraStatBonuses.Contains(OldPair.Key))
		{
			MyStats.ExtraStats.FindOrAdd(OldPair.Key) -= OldPair.Value;
		}
	}
	CyclePhaseExtraStatBonuses = NewCyclePhaseExtraStatBonuses;

	// 3. 現在HPの変化前状態を記憶
	bool bWasFullHP = (MyStats.HP >= MyStats.MaxHP);

	// 4. ステータスの確定（基礎値 ＋ 装備ボーナス）
	// ※ 装備の「HP」補正は最大HP（MaxHP）への加算として扱う
	MyStats.MaxHP = BaseMaxHP + GetBonus(ETargetStat::HP);
	MyStats.STR = BaseSTR + GetBonus(ETargetStat::STR);
	MyStats.VIT = BaseVIT + GetBonus(ETargetStat::VIT);
	MyStats.DEX = BaseDEX + GetBonus(ETargetStat::DEX);
	MyStats.AGI = BaseAGI + GetBonus(ETargetStat::AGI);
	MyStats.Mental = 1.0f + MyStats.MentalBonus + GetBonus(ETargetStat::Mental); // 初期値1.0 + 恒久加算分 + 装備ボーナス

	// 5. STRやVITから派生する戦闘力（攻撃力・防御力）を計算
	// ※AttackPower/DefensePower自体はRecalculateFatigueAdjustedCombatStats()が疲労補正込みで確定させるので、
	//   ここではBase側（疲労補正前の素の値）だけを更新する
	MyStats.BaseAttackPower = MyStats.STR * 2.0f + GetBonus(ETargetStat::AttackPower);
	MyStats.BaseDefensePower = (MyStats.VIT * 2.0f) + GetBonus(ETargetStat::DefensePower); // 装備のDEFはここに直接足す
	MyStats.Accuracy = MyStats.DEX * 1.5f + GetBonus(ETargetStat::Accuracy);
	MyStats.Evasion = MyStats.AGI * 1.5f + GetBonus(ETargetStat::Evasion);

	// 6. HPの補正（元々満タンなら満タンを維持、最大値を超えていたら丸める）
	if (bWasFullHP)
	{
		MyStats.HP = MyStats.MaxHP;
	}
	else if (MyStats.HP > MyStats.MaxHP)
	{
		MyStats.HP = MyStats.MaxHP;
	}

	// 6.5. Base側（STR/VIT+装備ボーナス）が確定したので、疲労補正込みのAttackPower/DefensePowerを計算する
	RecalculateFatigueAdjustedCombatStats();

	// 7. UIへ通知（ステータス画面やHPバーの更新）
	if (OnHPChangedDelegate.IsBound())
	{
		OnHPChangedDelegate.Broadcast(MyStats.HP, MyStats.MaxHP);
	}
	OnHPChanged(MyStats.HP, MyStats.MaxHP);
	NotifyStatsChanged();
}

FName AMyProject1Character::GetEquippedItemID(EEquipmentSlot Slot)
{
	// Mapの中に、指定されたスロット（部位）のデータが存在するかチェック
	if (CurrentEquippedItems.Contains(Slot))
	{
		// 存在すれば、そのアイテムID（FName）を返す
		return CurrentEquippedItems[Slot];
	}

	// 登録されていなければ（何も装備していなければ） None を返す
	return NAME_None;
}

void AMyProject1Character::ApplyShoesOffset(bool bEquip, float Offset, FRotator RotationOffset, FRotator ToeRotation)
{
	// メッシュが存在しない場合はエラーになるので弾く
	if (!GetMesh()) return;

	if (bEquip)
	{
		// ★履き替え対策：すでに別の靴を履いていて高さが変わっているなら、一旦元に戻す
		if (CurrentShoesOffset != 0.0f)
		{
			FVector ResetLoc = GetMesh()->GetRelativeLocation();
			ResetLoc.Z -= CurrentShoesOffset;
			GetMesh()->SetRelativeLocation(ResetLoc);
		}

		// 新しい靴の高さを適用する
		if (Offset != 0.0f)
		{
			FVector NewLoc = GetMesh()->GetRelativeLocation();
			NewLoc.Z += Offset;
			GetMesh()->SetRelativeLocation(NewLoc);
		}


		CurrentShoesOffset = Offset;
		CurrentAnkleRotationOffset = RotationOffset;

		if (ToeRotation == FRotator::ZeroRotator && RotationOffset != FRotator::ZeroRotator)
		{
			CurrentToeRotationOffset = RotationOffset * -1.0f;
		}
		else
		{
			// データテーブルで個別に設定されている場合は、その数値を優先する
			CurrentToeRotationOffset = ToeRotation;
		}
	}
	else
	{
		// 【靴を脱いだ時】メッシュを元の高さに戻す
		if (CurrentShoesOffset != 0.0f)
		{
			FVector NewLoc = GetMesh()->GetRelativeLocation();
			NewLoc.Z -= CurrentShoesOffset;
			GetMesh()->SetRelativeLocation(NewLoc);

			// 記憶をリセット
			CurrentShoesOffset = 0.0f;
		}

		CurrentAnkleRotationOffset = FRotator::ZeroRotator;
		CurrentToeRotationOffset = FRotator::ZeroRotator;
	}
}

void AMyProject1Character::UpdateCycleState()
{
	// プレイヤー以外（敵やNPC）は計算しなくてよい場合は弾く
	if (!IsPlayerControlled()) return;

	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	// 1. TotalElapsedDays から「1〜30」などの数値を割り出す
	// 💡全体のサイクル日数も、エディタ側で設定されたルールの一番大きい終了日から自動計算するようにします
	int32 MaxCycleDays = 30; // ルールが空の時のための保険のデフォルト値
	if (GameInst->CyclePhaseRules.Num() > 0)
	{
		// リストの最後の要素の MaxDay を全体のサイクル日数とする（例：最後の要素が「21〜30」なら30日サイクル）
		MaxCycleDays = GameInst->CyclePhaseRules.Last().MaxDay;
	}

	CurrentCycleDay = (GameInst->TotalElapsedDays % MaxCycleDays) + 1;

	// 古い状態を記憶しておく（切り替わった時だけログを出すため）
	ECycleState OldState = CurrentCycleState;

	// 2.エディタで設定したルールを上から順にチェックする！
	bool bFoundMatchingState = false;
	FText NewPhaseMessage;

	for (const FCyclePhaseSettings& Rule : GameInst->CyclePhaseRules)
	{
		// 今の日数が、設定された「MinDay 〜 MaxDay」の範囲内に入っているかチェック
		if (CurrentCycleDay >= Rule.MinDay && CurrentCycleDay <= Rule.MaxDay)
		{
			CurrentCycleState = Rule.TargetState;
			GameInst->CurrentCycleState = Rule.TargetState;
			NewPhaseMessage = Rule.PhaseChangeMessage;
			bFoundMatchingState = true;
			break; // 一致するものが見つかったのでループを抜ける
		}
	}

	// もしエディタの設定ミスなどでどの範囲にも当てはまらなかった場合の保険
	if (!bFoundMatchingState)
	{
		CurrentCycleState = ECycleState::StateA;
		GameInst->CurrentCycleState = ECycleState::StateA;
	}

	// 3. もし状態が切り替わったら、ステータス補正を再計算してログでお知らせ（メッセージ・補正はエディタのCyclePhaseRulesで設定）
	if (OldState != CurrentCycleState)
	{
		RefreshEquipmentStats();

		if (!NewPhaseMessage.IsEmpty())
		{
			OnReceiveLogMessage(NewPhaseMessage.ToString(), ELogMessageType::System);
		}
	}
}

float AMyProject1Character::GetExtraStat(FName StatName) const
{
	// マップの中にキーが存在するかチェックし、あればその値を返す
	if (const float* FoundValue = MyStats.ExtraStats.Find(StatName))
	{
		return *FoundValue;
	}
	return 0.0f; // 存在しない場合は0を返す
}

void AMyProject1Character::SetExtraStat(FName StatName, float Value)
{
	// 値を追加、または既存のキーがあれば上書きする
	MyStats.ExtraStats.Add(StatName, Value);

	// 値が変更されたので、モーフターゲットの更新処理を呼ぶ
	UpdateMorphTargetFromStat(StatName, Value);

	// ステータス画面などのUIへ通知（既存のシステムを流用）
	NotifyStatsChanged();
}

void AMyProject1Character::AddExtraStat(FName StatName, float Amount)
{
	// 現在の値を取得して加算し、SetExtraStatに流す
	float CurrentValue = GetExtraStat(StatName);
	SetExtraStat(StatName, CurrentValue + Amount);
}

void AMyProject1Character::UpdateMorphTargetFromStat(FName StatName, float Value)
{
	// メッシュが存在しない場合は処理しない
	if (!GetMesh()) return;

	// このStatNameが、モーフ連動設定リストに登録されているかチェック
	if (FExtraStatMorphConfig* MorphConfig = ExtraStatMorphLinks.Find(StatName))
	{
		// 登録されていれば、名前が空でないか確認
		if (!MorphConfig->MorphTargetName.IsNone())
		{
			// 0割り防止
			float MaxVal = MorphConfig->MaxStatValue > 0.0f ? MorphConfig->MaxStatValue : 1.0f;

			// ステータスの値を 0.0 ～ 1.0 のモーフ用割合に変換する
			// 例：Valueが50、Maxが100 なら 50 / 100 = 0.5f になる
			float MorphRatio = FMath::Clamp(Value / MaxVal, 0.0f, 1.0f);

			// 本体のメッシュにモーフを適用する
			GetMesh()->SetMorphTarget(MorphConfig->MorphTargetName, MorphRatio);

			// ★重要：LeaderPoseComponentを使用している場合、
			// 基本的には本体(GetMesh)のモーフを動かせば、同期している装備(服など)のモーフも自動連動しますが、
			// もし連動しないパーツがある場合は、ここで個別に SetMorphTarget を呼んでください。
			/*
			if (TorsoMeshComp) TorsoMeshComp->SetMorphTarget(MorphConfig->MorphTargetName, MorphRatio);
			if (HeadMeshComp)  HeadMeshComp->SetMorphTarget(MorphConfig->MorphTargetName, MorphRatio);
			*/
		}
	}
}

// ==========================================
// 鎖（ケーブル）システムの実装群
// ==========================================

void AMyProject1Character::ClearCableSystem(EEquipmentSlot Slot)
{
	// 手・腕スロットかどうかの判定
	bool bIsHands = (Slot == EEquipmentSlot::Hands || Slot == EEquipmentSlot::Wrist);

	if (bIsHands)
	{
		CurrentCableSourceSocket_Hands = NAME_None;
		CurrentCableSourceComponent_Hands = nullptr;
		CurrentCableTargetSocket_Hands = NAME_None;
		CurrentCableTargetComponent_Hands = nullptr;

		if (EquipmentCableComp_Hands)
		{
			EquipmentCableComp_Hands->SetVisibility(false);
			EquipmentCableComp_Hands->SetHiddenInGame(true);
			EquipmentCableComp_Hands->Deactivate();            // 物理シミュレーションを止める
			EquipmentCableComp_Hands->CableLength = 0.0f;       // 長さを0にして潰す
			EquipmentCableComp_Hands->bAttachEnd = false;

			// 親を中継ダミーからキャラクター本体のMeshに強制アタッチで引き戻す！
			EquipmentCableComp_Hands->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			EquipmentCableComp_Hands->SetAttachEndToComponent(nullptr, NAME_None);
			EquipmentCableComp_Hands->EndLocation = FVector::ZeroVector;
		}
	}
	else
	{
		CurrentCableSourceSocket = NAME_None;
		CurrentCableSourceComponent = nullptr;
		CurrentCableTargetSocket = NAME_None;
		CurrentCableTargetComponent = nullptr;

		if (EquipmentCableComp)
		{
			EquipmentCableComp->SetVisibility(false);
			EquipmentCableComp->SetHiddenInGame(true);
			EquipmentCableComp->Deactivate();                  // 物理シミュレーションを止める
			EquipmentCableComp->CableLength = 0.0f;             // 長さを0にして潰す
			EquipmentCableComp->bAttachEnd = false;

			// 親を中継ダミーからキャラクター本体のMeshに強制アタッチで引き戻す！
			EquipmentCableComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			EquipmentCableComp->SetAttachEndToComponent(nullptr, NAME_None);
			EquipmentCableComp->EndLocation = FVector::ZeroVector;
		}
	}

	if (!bIsHands && WeaponSubMeshComp)
	{
		WeaponSubMeshComp->SetVisibility(false);
		WeaponSubMeshComp->SetSimulatePhysics(false);
		WeaponSubMeshComp->SetStaticMesh(nullptr);
		WeaponSubMeshComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	}
}

void AMyProject1Character::SetupCableSystem(const FCableAttachmentSettings& Settings, EEquipmentSlot Slot, USceneComponent* AssociatedComponent)
{
	// まず指定されたスロットの鎖を一度リセット
	ClearCableSystem(Slot);

	bool bIsHands = (Slot == EEquipmentSlot::Hands || Slot == EEquipmentSlot::Wrist);

	// 操作対象のコンポーネントとダミーをスロットに応じて切り替える
	UCableComponent* TargetCable = bIsHands ? EquipmentCableComp_Hands : EquipmentCableComp;
	USceneComponent* TargetDummyStart = bIsHands ? CableDummyStart_Hands : CableDummyStart;
	USceneComponent* TargetDummyEnd = bIsHands ? CableDummyEnd_Hands : CableDummyEnd;

	if (Settings.AttachType == ECableAttachType::None || !TargetCable) return;

	UMaterialInterface* LoadedMaterial = Settings.CableMaterial.IsNull() ? nullptr : Settings.CableMaterial.LoadSynchronous();

	// 眠らせたコンポーネントを完全に再起動
	TargetCable->Activate(true);
	TargetCable->SetHiddenInGame(false);

	// --- 鎖パラメータ適用 ---
	TargetCable->CableWidth = Settings.CableThickness;
	TargetCable->CableLength = Settings.CableLength;
	TargetCable->TileMaterial = FMath::Max(1.0f, Settings.CableLength / 6.0f);
	TargetCable->SolverIterations = 32;

	if (LoadedMaterial)
	{
		if (TargetCable->OverrideMaterials.Num() == 0) TargetCable->OverrideMaterials.Add(LoadedMaterial);
		else TargetCable->OverrideMaterials[0] = LoadedMaterial;
		TargetCable->MarkRenderStateDirty();
	}
	TargetCable->SetVisibility(true);

	// 変数の格納先を分岐
	if (bIsHands)
	{
		CurrentCableSourceSocket_Hands = Settings.SourceSocketName;
	}
	else
	{
		CurrentCableSourceSocket = Settings.SourceSocketName;
	}

	switch (Settings.AttachType)
	{
	case ECableAttachType::WeaponToSub:
	{
		if (!bIsHands && WeaponSubMeshComp && AssociatedComponent)
		{
			WeaponSubMeshComp->SetVisibility(true);
			CurrentCableSourceComponent = AssociatedComponent;
			TargetCable->AttachToComponent(TargetDummyStart, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			TargetCable->bAttachEnd = true;
			TargetCable->SetAttachEndToComponent(WeaponSubMeshComp, Settings.TargetSocketName);
			WeaponSubMeshComp->SetSimulatePhysics(true);
		}
	}
	break;

	case ECableAttachType::LeftToRight:
	{
		USceneComponent* BestComp = AssociatedComponent ? AssociatedComponent : GetMesh();
		USceneComponent* FinalSourceComp = GetMesh();
		USceneComponent* FinalTargetComp = GetMesh();

		if (USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(BestComp))
		{
			if (SkelComp->DoesSocketExist(Settings.SourceSocketName)) FinalSourceComp = SkelComp;
			if (SkelComp->DoesSocketExist(Settings.TargetSocketName)) FinalTargetComp = SkelComp;
		}

		if (bIsHands)
		{
			CurrentCableSourceComponent_Hands = FinalSourceComp;
			CurrentCableTargetComponent_Hands = FinalTargetComp;
			CurrentCableTargetSocket_Hands = Settings.TargetSocketName;
		}
		else
		{
			CurrentCableSourceComponent = FinalSourceComp;
			CurrentCableTargetComponent = FinalTargetComp;
			CurrentCableTargetSocket = Settings.TargetSocketName;
		}

		// 鎖の始点をそれぞれのダミーStartに接続
		TargetCable->AttachToComponent(TargetDummyStart, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		// 鎖の終点をそれぞれのダミーEndに正規接続
		TargetCable->bAttachEnd = true;
		TargetCable->SetAttachEndToComponent(TargetDummyEnd, NAME_None);
	}
	break;

	case ECableAttachType::ToWorldObject:
	{
		if (!bIsHands && WeaponSubMeshComp)
		{
			WeaponSubMeshComp->SetVisibility(true);
			CurrentCableSourceComponent = GetMesh();
			WeaponSubMeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			WeaponSubMeshComp->SetSimulatePhysics(true);
			TargetCable->AttachToComponent(TargetDummyStart, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			TargetCable->bAttachEnd = true;
			TargetCable->SetAttachEndToComponent(WeaponSubMeshComp, Settings.TargetSocketName);
		}
	}
	break;

	default:
		break;
	}

	// 初回位置の強制同期
	if (bIsHands)
	{
		if (CurrentCableSourceComponent_Hands && !CurrentCableSourceSocket_Hands.IsNone() && TargetDummyStart)
		{
			TargetDummyStart->SetWorldLocation(CurrentCableSourceComponent_Hands->GetSocketLocation(CurrentCableSourceSocket_Hands));
		}
		if (CurrentCableTargetComponent_Hands && !CurrentCableTargetSocket_Hands.IsNone() && TargetDummyEnd)
		{
			TargetDummyEnd->SetWorldLocation(CurrentCableTargetComponent_Hands->GetSocketLocation(CurrentCableTargetSocket_Hands));
		}
	}
	else
	{
		if (CurrentCableSourceComponent && !CurrentCableSourceSocket.IsNone() && TargetDummyStart)
		{
			TargetDummyStart->SetWorldLocation(CurrentCableSourceComponent->GetSocketLocation(CurrentCableSourceSocket));
		}
		if (CurrentCableTargetComponent && !CurrentCableTargetSocket.IsNone() && TargetDummyEnd)
		{
			TargetDummyEnd->SetWorldLocation(CurrentCableTargetComponent->GetSocketLocation(CurrentCableTargetSocket));
		}
	}
}

void AMyProject1Character::TryAddSkinOverlay(FName RowName, bool bIsShopPurchase, int32 OverridePrice, FString DisplayName, EShopModeCategory ShopCategory)
{
	if (!SkinOverlayComp || RowName.IsNone()) return;

	// ピアスはDT_Equipments（装備）へ統合済み。旧オーバーレイ方式ではなく装備の着脱で処理する。
	if (ShopCategory == EShopModeCategory::Piercing)
	{
		const int32 Markup = ActiveShopNPC ? ActiveShopNPC->PiercingAddMarkup : 10000;
		TryDoctorAddPiercing(RowName, Markup);
		return;
	}

	// 拘束具・呪物は「購入」概念がない除去専用ショップ。ここに来ても何もしない
	// （GetOverlayDataTableByCategory の default が Tattoo テーブルを返すため、素通りさせると誤課金＋誤オーバーレイになる）。
	if (ShopCategory == EShopModeCategory::Restraint)
	{
		return;
	}

	if (SkinOverlayComp->IsOverlayActive(RowName, ShopCategory))
	{
		if (bIsShopPurchase)
		{
			OnReceiveLogMessage(TEXT("それはすでに体に刻まれているか、既に治療・装着済みです。"), ELogMessageType::System);
		}
		return;
	}

	int32 FinalPrice = OverridePrice;
	if (bIsShopPurchase && FinalPrice <= 0)
	{
		UDataTable* TargetDT = SkinOverlayComp->GetOverlayDataTableByCategory(ShopCategory);
		if (TargetDT)
		{
			if (ShopCategory == EShopModeCategory::Tattoo)
			{
				FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(RowName, TEXT("ShopAddPriceLookup"));
				if (Data) FinalPrice = Data->BuyPrice;
			}
			else if (ShopCategory == EShopModeCategory::Scar || ShopCategory == EShopModeCategory::Disease)
			{
				// 傷跡や病気を「お金を払って新規追加（購入）」するショップ仕様はないため0円
				FinalPrice = 0;
			}
		}
	}

	if (InventoryComp)
	{
		if (!InventoryComp->TrySpendGil(FinalPrice))
		{
			if (bIsShopPurchase)
			{
				OnReceiveLogMessage(TEXT("お金（円）が足りません。"), ELogMessageType::System);
			}
			return;
		}
	}

	SkinOverlayComp->AddOverlay(RowName, -1.0f, ShopCategory);

	if (OnSkinOverlayUIChangedDelegate.IsBound())
	{
		OnSkinOverlayUIChangedDelegate.Broadcast();
	}

	FString TargetName = DisplayName.IsEmpty() ? RowName.ToString() : DisplayName;
	if (bIsShopPurchase)
	{
		if (ShopCategory == EShopModeCategory::Disease) OnReceiveLogMessage(FString::Printf(TEXT("【%s】を治療した。%d円を支払った。"), *TargetName, FinalPrice), ELogMessageType::System);
		else if (ShopCategory == EShopModeCategory::Scar) OnReceiveLogMessage(FString::Printf(TEXT("【%s】を綺麗に整形した。%d円を支払った。"), *TargetName, FinalPrice), ELogMessageType::System);
		else OnReceiveLogMessage(FString::Printf(TEXT("体に新たな刺青【%s】を刻んだ。%d円を支払った。"), *TargetName, FinalPrice), ELogMessageType::System);
	}
	else
	{
		if (ShopCategory == EShopModeCategory::Disease) OnReceiveLogMessage(FString::Printf(TEXT("【%s】が治癒した。"), *TargetName), ELogMessageType::System);
		else if (ShopCategory == EShopModeCategory::Scar) OnReceiveLogMessage(FString::Printf(TEXT("【%s】が綺麗に整形された。"), *TargetName), ELogMessageType::System);
		else OnReceiveLogMessage(FString::Printf(TEXT("体に新たな刺青【%s】が刻まれた。"), *TargetName), ELogMessageType::System);
	}
}

void AMyProject1Character::TryRemoveSkinOverlay(FName RowName, bool bIsShopPurchase, int32 OverridePrice, FString DisplayName, EShopModeCategory ShopCategory)
{
	if (!SkinOverlayComp) return;

	// ピアスはDT_Equipments（装備）へ統合済み。呪われピアスの除去は装備側で処理する。
	if (ShopCategory == EShopModeCategory::Piercing)
	{
		const int32 ShopLevel = ActiveShopNPC ? ActiveShopNPC->ShopLevel : 99;
		TryDoctorRemovePiercing(RowName, ShopLevel);
		return;
	}

	// 拘束具・呪物もDT_Equipments（装備）側。RowNameから装備中スロットを解決してTryShopUnlockRestraintへ委譲。
	// レベル判定・ギル支払い・インベントリへの返却・ログは TryShopUnlockRestraint がすべて行う。
	if (ShopCategory == EShopModeCategory::Restraint)
	{
		const int32 ShopLevel = ActiveShopNPC ? ActiveShopNPC->ShopLevel : 99;
		for (const TPair<EEquipmentSlot, FName>& Pair : CurrentEquippedItems)
		{
			if (Pair.Value == RowName && !IsPiercingSlot(Pair.Key))
			{
				const EEquipmentSlot SlotToUnlock = Pair.Key;
				if (TryShopUnlockRestraint(SlotToUnlock, ShopLevel) && OnSkinOverlayUIChangedDelegate.IsBound())
				{
					// 施術ショップWBPが購読しているUI更新デリゲート。解除成功時にリストを再描画させる。
					OnSkinOverlayUIChangedDelegate.Broadcast();
				}
				return;
			}
		}
		return;
	}

	if (!SkinOverlayComp->IsOverlayActive(RowName, ShopCategory))
	{
		if (bIsShopPurchase)
		{
			OnReceiveLogMessage(TEXT("消去・取り外す対象が体に見当たりません。"), ELogMessageType::System);
		}
		return;
	}

	int32 FinalPrice = OverridePrice;
	if (bIsShopPurchase && FinalPrice <= 0)
	{
		UDataTable* TargetDT = SkinOverlayComp->GetOverlayDataTableByCategory(ShopCategory);
		if (TargetDT)
		{
			if (ShopCategory == EShopModeCategory::Tattoo)
			{
				FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(RowName, TEXT("ShopRemovePriceLookup"));
				if (Data) FinalPrice = Data->RemovePrice;
			}
			else if (ShopCategory == EShopModeCategory::Scar)
			{
				FScarDataRow* Data = TargetDT->FindRow<FScarDataRow>(RowName, TEXT("ShopRemovePriceLookup"));
				if (Data) FinalPrice = Data->RemovePrice;
			}
			else if (ShopCategory == EShopModeCategory::Disease)
			{
				// 病気専用の構造体から、治療費（TreatmentPrice）を「除去コスト」として正しく引っこ抜く！
				FDiseaseTreatmentDataRow* Data = TargetDT->FindRow<FDiseaseTreatmentDataRow>(RowName, TEXT("ShopRemovePriceLookup"));
				if (Data) FinalPrice = Data->TreatmentPrice;
			}
		}
	}

	if (InventoryComp)
	{
		if (!InventoryComp->TrySpendGil(FinalPrice))
		{
			if (bIsShopPurchase)
			{
				OnReceiveLogMessage(TEXT("費用（円）が足りません。"), ELogMessageType::System);
			}
			return;
		}
	}

	SkinOverlayComp->RemoveOverlay(RowName, ShopCategory);

	if (OnSkinOverlayUIChangedDelegate.IsBound())
	{
		OnSkinOverlayUIChangedDelegate.Broadcast();
	}

	FString TargetName = DisplayName.IsEmpty() ? RowName.ToString() : DisplayName;
	if (bIsShopPurchase)
	{
		if (ShopCategory == EShopModeCategory::Disease) OnReceiveLogMessage(FString::Printf(TEXT("【%s】の治療が完了した。%d円を支払った。"), *TargetName, FinalPrice), ELogMessageType::System);
		else OnReceiveLogMessage(FString::Printf(TEXT("体から【%s】を消去した。%d円を支払った。"), *TargetName, FinalPrice), ELogMessageType::System);
	}
	else
	{
		if (ShopCategory == EShopModeCategory::Disease) OnReceiveLogMessage(FString::Printf(TEXT("【%s】が完治した。"), *TargetName), ELogMessageType::System);
		else OnReceiveLogMessage(FString::Printf(TEXT("体から【%s】が消滅した。"), *TargetName), ELogMessageType::System);
	}
}

bool AMyProject1Character::TryBuyItem(FName ItemID, int32 Amount)
{
	// 安全性のチェック（インベントリがない、またはIDが不正、個数が0以下なら弾く）
	if (!InventoryComp || ItemID.IsNone() || Amount <= 0) return false;

	// 現在開いているショップがこのアイテムを取り扱っていないなら弾く
	if (ActiveShopNPC && !ActiveShopNPC->CanBuyItem(ItemID))
	{
		OnReceiveLogMessage(TEXT("このお店では扱っていない品です。"), ELogMessageType::System);
		return false;
	}

	// データテーブルからアイテムの基本情報を取得
	FItemData ItemInfo;
	if (!InventoryComp->GetItemDataBP(ItemID, ItemInfo)) return false;

	// 合計金額の計算
	int32 TotalPrice = ItemInfo.Price * Amount;

	// 1. 所持金（ギル）が足りているかチェック
	if (InventoryComp->Gil < TotalPrice)
	{
		OnReceiveLogMessage(TEXT("お金（円）が足りません。"), ELogMessageType::System);
		return false;
	}

	// 2. アイテムがカバンに入るかチェック（AddItemを実行して空きスロットを確認）
	// AddItemはカバンがいっぱいなら自動でfalseを返し、入るなら true を返して内部データを更新します
	if (InventoryComp->AddItem(ItemID, Amount))
	{
		// 3. アイテムの追加に成功したため、代金を支払う（ギルを減らす）
		InventoryComp->TrySpendGil(TotalPrice);

		// 4. 治療屋と同じスタイルでシステムログを出力
		FString LogMsg = FString::Printf(TEXT("%sを%d個購入した。%d円を支払った。"), *ItemInfo.Name, Amount, TotalPrice);
		OnReceiveLogMessage(LogMsg, ELogMessageType::System);

		// 5. 購入成立時のお金のSEを再生（AddItem内の入手音とは別枠。エディタでセットされていなければ何もしない）
		if (ShopPurchaseSound)
		{
			UGameplayStatics::PlaySound2D(this, ShopPurchaseSound);
		}

		return true;
	}
	else
	{
		// カバンの容量（MaxSlots）が限界で入らなかった場合
		OnReceiveLogMessage(TEXT("カバンがいっぱいでこれ以上買えません。"), ELogMessageType::System);
		return false;
	}
}

bool AMyProject1Character::TrySellItem(FName ItemID, int32 Amount)
{
	if (!InventoryComp || ItemID.IsNone() || Amount <= 0) return false;

	// 現在開いているショップがこのアイテムを買い取らないなら弾く
	if (ActiveShopNPC && !ActiveShopNPC->CanSellItem(ItemID))
	{
		OnReceiveLogMessage(TEXT("このお店では買い取ってもらえない品です。"), ELogMessageType::System);
		return false;
	}

	FItemData ItemInfo;
	if (!InventoryComp->GetItemDataBP(ItemID, ItemInfo)) return false;

	// 1. 「だいじなもの（KeyItem）」とEXアイテム（イベント/会話の受け渡しでのみ増減する特別な品）は売却不可
	if (ItemInfo.ItemType == EItemType::KeyItem || ItemInfo.bIsEx)
	{
		OnReceiveLogMessage(TEXT("それは売却できません。"), ELogMessageType::System);
		return false;
	}
	// 現在いずれかのスロットに装備中のアイテムは売却できないように安全にブロックする
	for (const auto& Pair : CurrentEquippedItems)
	{
		// 装備中のアイテムID（Pair.Value）が、売ろうとしたItemIDと一致した場合
		if (Pair.Value == ItemID)
		{
			OnReceiveLogMessage(TEXT("装備中のアイテムは売却できません。"), ELogMessageType::System);
			return false; // ここで処理を強制終了して売らせない
		}
	}

	// 2. プレイヤーが指定された個数以上のアイテムを本当に持っているかチェック
	if (InventoryComp->GetItemQuantity(ItemID) < Amount)
	{
		OnReceiveLogMessage(TEXT("売却するアイテムが足りません。"), ELogMessageType::System);
		return false;
	}

	// 売却で得られる合計金額の計算
	int32 TotalSellPrice = ItemInfo.SellPrice * Amount;

	// 3. カバンからアイテムを削除する
	if (InventoryComp->RemoveItem(ItemID, Amount))
	{
		// 4. 削除に成功したら、売却代金をプレイヤーの財布に加算する
		InventoryComp->AddGil(TotalSellPrice);

		// システムログを出力
		FString LogMsg = FString::Printf(TEXT("%sを%d個売却した。%d円を手に入れた。"), *ItemInfo.Name, Amount, TotalSellPrice);
		OnReceiveLogMessage(LogMsg, ELogMessageType::System);

		// 売却成立時のお金のSEを再生（エディタでセットされていなければ何もしない）
		if (ShopSellSound)
		{
			UGameplayStatics::PlaySound2D(this, ShopSellSound);
		}

		return true;
	}

	return false;
}