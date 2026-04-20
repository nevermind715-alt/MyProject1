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
#include "RpgDamageCalculator.h" // ★これを必ず追加！
#include "Engine/LocalPlayer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h" // これを追加
#include "GameFramework/Pawn.h"     // これも念のため
#include "GameFramework/CharacterMovementComponent.h" // ←これを追加
#include "Kismet/KismetSystemLibrary.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "MyProject1HUD.h"
#include "Blueprint/UserWidget.h"
#include "QuestComponent.h"
#include "DialogComponent.h"
#include "MyProject1GameInstance.h"
#include "MusicControlComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PhysicsVolume.h"


AMyProject1Character::AMyProject1Character()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// --- 武器コンポーネントの作成と設定（ここで行う） ---
	WeaponMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComp"));
	// キャラクターの手にアタッチ（ソケット名はスケルトンに合わせて調整）
	WeaponMeshComp->SetupAttachment(GetMesh(), FName("hand_r_socket"));

	// StaticMesh用の武器コンポーネント
	StaticWeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMeshComponent"));
	StaticWeaponMeshComp->SetupAttachment(GetMesh(), FName("hand_r_socket"));
	StaticWeaponMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 武器自体が何かにぶつかってバグるのを防ぐ

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
	QuestComp = CreateDefaultSubobject<UQuestComponent>(TEXT("QuestComp"));
	DialogComp = CreateDefaultSubobject<UDialogComponent>(TEXT("DialogComp"));
	MusicComp = CreateDefaultSubobject<UMusicControlComponent>(TEXT("MusicComp"));

}

// --- BeginPlay の実装 ---
void AMyProject1Character::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// エディタ側で変数 DefaultMappingContext に IMC_Default などをセットしておく必要があります
			// もし変数がなければ、まずは BP 側で Add Mapping Context ノードを組む形でも動きます
		}
	}

	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
	{
		// すぐに呼ぶのではなく、1フレーム待つか、
		// あるいは確実に Controller が設定されていることを確認してから呼ぶ
		if (GetController())
		{
			GameInst->ApplyPendingWarp(this);
		}

		// マップも太陽も準備完了したこのタイミングで、初回の時間を通知する ---
		if (GameInst->OnInGameTimeChanged.IsBound())
		{
			int32 Hour = GameInst->CurrentTimeInMinutes / 60;
			int32 Minute = GameInst->CurrentTimeInMinutes % 60;
			GameInst->OnInGameTimeChanged.Broadcast(GameInst->CurrentYear, GameInst->CurrentMonth, GameInst->CurrentDay, Hour, Minute);
		}

	}

	// ゲーム開始時にデータテーブルの情報をキャラクターに反映させる
	ApplyJobData();

	UpdateHealthWidgetName(CharacterName);

	LastCombatTime = GetWorld()->GetTimeSeconds();

	if (AutoRecoveryInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_AutoRecovery, this, &AMyProject1Character::HandleAutoRecovery, AutoRecoveryInterval, true);
	}

	// 疲労度の計算を「1秒に1回」のペースで自動実行する
	GetWorldTimerManager().SetTimer(TimerHandle_FatigueUpdate, this, &AMyProject1Character::HandleFatigueTick, 1.0f, true);
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

		

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyProject1Character::OnToggleMenuPressed);

	}
	
	
}

void AMyProject1Character::OnToggleMenuPressed()
{
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
	// ※ ここで "APawn::StaticClass()" としていますが、敵専用クラスがあるなら "ACombatEnemy::StaticClass()" にするとより正確です
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundActors);

	AActor* ClosestActor = nullptr;
	float MinDistance = TargetingRange; // 最初は最大射程をセット

	// 2. 見つかった全員をチェック
	for (AActor* Actor : FoundActors)
	{
		// 自分自身はターゲットしない
		if (Actor == this || !Actor) continue;

		// ★追加：死んでる敵は無視する
		AMyProject1Character* TargetChar = Cast<AMyProject1Character>(Actor);
		if (TargetChar && TargetChar->IsDead()) continue;

		// 距離を測る
		float Dist = GetDistanceTo(Actor);

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
	// 1. 周囲の敵を全員探す
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundActors);

	// 2. 「射程内」の敵だけのリストを作る
	TArray<AActor*> ValidTargets;
	for (AActor* Actor : FoundActors)
	{
		if (Actor == this || !Actor) continue;

		// 死んでる敵は無視する
		AMyProject1Character* TargetChar = Cast<AMyProject1Character>(Actor);
		if (TargetChar && TargetChar->IsDead()) continue;

		if (GetDistanceTo(Actor) <= TargetingRange) // 射程内か？
		{
			ValidTargets.Add(Actor);
		}
	}

	// 敵がいなければ終了
	if (ValidTargets.Num() == 0) return;

	// 3. もし今のターゲットがいなければ、一番近い敵を選ぶ（TargetNearestと同じ動き）
	if (CurrentTarget == nullptr)
	{
		TargetNearestEnemy();
		return;
	}

	// 4. 今のターゲットがリストの「何番目」にいるか探す
	int32 CurrentIndex = ValidTargets.Find(CurrentTarget);

	// 5. 次の番号を計算する（最後の番号なら0番に戻る）
	int32 NextIndex;
	if (CurrentIndex == INDEX_NONE)
	{
		// リストに見つからなかったら0番目（最初の敵）にする
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

	// --- 追加：周囲の敵のHPバー表示ロジック ---
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

	// --- 1. 死んでいる場合は何もしない ---
	if (IsDead()) return;

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

	// --- 4. 最終的な速度を一括で適用する ---
	if (GetCharacterMovement()->MaxWalkSpeed != TargetSpeed)
	{
		GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
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
		if (IsPlayerControlled() && MusicComp)
		{
			MusicComp->SetCombatMusicActive(false);
		}
		return; // ★重要：ターゲットがいない時はここで処理を終わらせる（速度設定は上で終わっているので安全！）
	}

	// --- 6. エリア離脱（ナビメッシュ外）の判定 ---
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys && CurrentTarget)
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
		if (Attacker != this)
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

	// 2. 物理挙動とコリジョン停止（BPエラー防止のため真っ先に止める）
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// 3. ラグドール化と削除予約
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);

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
	LastCombatTime = GetWorld()->GetTimeSeconds();

	// ジョブデータを取得
	FJobAttributes* JobData = JobRow.GetRow<FJobAttributes>(JobRow.RowName.ToString());

	// データがあり、かつモンタージュが1つ以上登録されているかチェック
	if (JobData && JobData->AttackMontages.Num() > 0)
	{
		// ★ランダムにインデックスを選択 (0 ～ 配列の最後)
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

	// 疲労度を加味した攻撃力で一時的に上書きする
	float OriginalAP = this->MyStats.AttackPower;
	this->MyStats.AttackPower = GetModifiedAttackPower();

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

	// 一時的に変えた攻撃力を元に戻す！（これが消えていました）
	this->MyStats.AttackPower = OriginalAP;

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
					// ★変更： \"%s\" を 【%s】 に変更
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
					// ★変更： \"%s\" を 【%s】 に変更
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
				// ★変更： \"%s\" を 【%s】 に変更
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
				// ★変更： \"%s\" を 【%s】 に変更
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
		if (Ally && Ally->GetClass() == this->GetClass() && Ally->CurrentTarget == nullptr)
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

	// 次のレベルへの必要経験値を増やす（例：1.2倍）
	MyStats.MaxXP = FMath::RoundToInt(MyStats.MaxXP * 1.2f);

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
		if (WeaponMeshComp)
		{
			if (!JobData->WeaponMesh.IsNull())
			{
				USkeletalMesh* NewMesh = JobData->WeaponMesh.LoadSynchronous();
				WeaponMeshComp->SetSkeletalMesh(NewMesh);

				// 大きさを反映
				WeaponMeshComp->SetRelativeScale3D(JobData->WeaponScale);

				// 強制的に手のソケットにアタッチ
				WeaponMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r_socket"));
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
					FName("hand_r_socket")
				);

				// ★ここからデバッグコードを追加：本当にソケットはあるか？★
				if (!GetMesh()->DoesSocketExist(FName("hand_r_socket")))
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


		// --- C. ステータスの再計算 ---
		MyStats.MaxHP = JobData->BaseHP + ((MyStats.Level - 1) * 15.0f);
		MyStats.STR = JobData->BaseSTR + ((MyStats.Level - 1) * 2.0f);
		MyStats.VIT = JobData->BaseVIT + ((MyStats.Level - 1) * 2.0f);
		MyStats.DEX = JobData->BaseDEX + ((MyStats.Level - 1) * 2.0f);
		MyStats.AGI = JobData->BaseAGI + ((MyStats.Level - 1) * 2.0f);

		MyStats.AttackPower = MyStats.STR * 2.0f;
		MyStats.DefensePower = MyStats.VIT * 2.0f;
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

	if (OnHPChangedDelegate.IsBound())
	{
		OnHPChangedDelegate.Broadcast(MyStats.HP, MyStats.MaxHP);
	}
	OnHPChanged(MyStats.HP, MyStats.MaxHP);

	NotifyStatsChanged();
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

		// ★ ここがポイント：準備状態を飛ばして「オートアタック中」を維持する
		bIsPreparingAttack = false;
		bIsAutoAttacking = true;

		
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

	AActor* BestTarget = nullptr;
	AActor* ClosestAggroTarget = nullptr; // 自分を狙っている敵用
	float MinDistAggro = TargetingRange;
	float MinDistAny = TargetingRange;

	for (AActor* Actor : FoundActors)
	{
		AMyProject1Character* PotentialEnemy = Cast<AMyProject1Character>(Actor);

		// 自分以外、かつ生きていて、射程内にいるか
		if (!PotentialEnemy || PotentialEnemy == this || PotentialEnemy->IsDead()) continue;

		float Dist = GetDistanceTo(PotentialEnemy);
		if (Dist > TargetingRange) continue;

		// 【優先度1】自分をターゲットにしている敵（アクティブな敵）
		if (PotentialEnemy->CurrentTarget == this)
		{
			if (Dist < MinDistAggro)
			{
				MinDistAggro = Dist;
				ClosestAggroTarget = PotentialEnemy;
			}
		}

		// 【優先度2】単に一番近い敵
		if (Dist < MinDistAny)
		{
			MinDistAny = Dist;
			BestTarget = PotentialEnemy;
		}
	}

	// 自分を狙っている敵がいればそちらを優先、いなければ一番近い敵を返す
	return ClosestAggroTarget ? ClosestAggroTarget : BestTarget;
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

void AMyProject1Character::ApplyItemBuff(FString ItemName, const TArray<FItemEffect>& Effects, float Duration)
{
	if (bIsDead || Duration <= 0.0f) return;

	// 1. すべての効果を一度に適用する
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
		case ETargetStat::AttackPower: MyStats.AttackPower += Effect.EffectAmount; break;
		case ETargetStat::Stamina:     MyStats.Stamina += Effect.EffectAmount;     break;
		default: break;
		}
	}

	// 2. タイマーを1つだけセットする
	FTimerHandle TimerHandle;
	FTimerDelegate Delegate;
	// ItemName と Effects 配列をタイマーの引数として渡す
	Delegate.BindUObject(this, &AMyProject1Character::ExpireItemBuff, ItemName, Effects);
	GetWorldTimerManager().SetTimer(TimerHandle, Delegate, Duration, false);

	// ※ アイテム使用時のログは InventoryComponent 側で1回出るので、ここでは出さない
}

void AMyProject1Character::ExpireItemBuff(FString ItemName, TArray<FItemEffect> Effects)
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
		case ETargetStat::AttackPower: MyStats.AttackPower -= Effect.EffectAmount; break;
		case ETargetStat::Stamina:     MyStats.Stamina -= Effect.EffectAmount;     break;
		default: break;
		}
	}

	// ★ ここでログを1回だけ出す
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

	// もし値が変動していたらUIに合図を送る
	if (MyStats.Energy != OldEnergy)
	{
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

	// 今回の1秒間で値が少しでも変わっていたら、UIを1回だけ更新する
	if (MyStats.Energy != OldEnergy)
	{
		NotifyStatsChanged();
	}
}

// --- 疲労度を加味した攻撃力の計算 ---
float AMyProject1Character::GetModifiedAttackPower() const
{
	float BaseAP = MyStats.AttackPower;

	if (!IsPlayerControlled()) return BaseAP;

	// ① まず一番重いペナルティ（90以上）をチェック
	if (MyStats.Energy >= FatigueThreshold2)
	{
		// デバフ②：攻撃力10%ダウン
		return BaseAP * (1.0f - FatigueAttackPenalty2);
	}
	// ② 次に中間のペナルティ（50以上〜90未満）をチェック
	else if (MyStats.Energy >= FatigueThreshold1)
	{
		// デバフ①：攻撃力5%ダウン
		return BaseAP * (1.0f - FatigueAttackPenalty1);
	}

	// 50未満ならそのままの攻撃力を返す
	return BaseAP;
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

void AMyProject1Character::AddFlag(FName FlagName)
{
	// フラグ名が空っぽではなく、まだ持っていない場合のみ追加する
	if (!FlagName.IsNone() && !MyStats.UnlockedFlags.Contains(FlagName))
	{
		MyStats.UnlockedFlags.Add(FlagName);
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
	if (!FlagName.IsNone())
	{
		MyStats.UnlockedFlags.Remove(FlagName);
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

