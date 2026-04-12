#include "MusicControlComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MusicManagerActor.h"
#include "EngineUtils.h"
#include "Math/UnrealMathUtility.h"

// 完全に0にするとUE5が強制リセットしてしまうため、人間の耳には聞こえない「0.01」を無音として扱います
const float SilentVolume = 0.01f;

UMusicControlComponent::UMusicControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMusicControlComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn != UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
	{
		return;
	}

	FieldAudioComp = NewObject<UAudioComponent>(GetOwner());
	FieldAudioComp->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	FieldAudioComp->bAutoDestroy = false;
	FieldAudioComp->RegisterComponent();
	FieldAudioComp->bAutoActivate = false;
	FieldAudioComp->bIsUISound = true;

	BattleAudioComp = NewObject<UAudioComponent>(GetOwner());
	BattleAudioComp->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	BattleAudioComp->bAutoDestroy = false;
	BattleAudioComp->RegisterComponent();
	BattleAudioComp->bAutoActivate = false;
	BattleAudioComp->bIsUISound = true;

	AMusicManagerActor* LevelMusicManager = nullptr;
	for (TActorIterator<AMusicManagerActor> It(GetWorld()); It; ++It)
	{
		LevelMusicManager = *It;
		break;
	}

	if (LevelMusicManager)
	{
		if (LevelMusicManager->FieldMusic)
		{
			FieldAudioComp->SetSound(LevelMusicManager->FieldMusic);
			FieldAudioComp->Play();
			CurrentFieldVolume = 1.0f;
		}

		if (LevelMusicManager->BattleMusic)
		{
			BattleAudioComp->SetSound(LevelMusicManager->BattleMusic);
			// 初期状態も完全に0ではなく SilentVolume にしておく
			BattleAudioComp->SetVolumeMultiplier(SilentVolume);
			CurrentBattleVolume = SilentVolume;
		}
	}
}

void UMusicControlComponent::SetCombatMusicActive(bool bIsCombat)
{
	if (!FieldAudioComp || !BattleAudioComp) return;
	if (bIsCombatMusicPlaying == bIsCombat) return;

	bIsCombatMusicPlaying = bIsCombat;

	if (bIsCombat)
	{
		// 【戦闘開始】目標の音量を SilentVolume（0.01）と 1.0 に設定
		TargetFieldVolume = SilentVolume;
		TargetBattleVolume = 1.0f;

		double CurrentTime = GetWorld()->GetTimeSeconds();
		if (CurrentTime - LastCombatEndTime <= CombatMusicResumeThreshold)
		{
			// 8秒以内：続きから
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("→ 8秒以内：続きから再生します！"));
			BattleAudioComp->SetPaused(false);
		}
		else
		{
			// 8秒経過：最初から（強制リセット）
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("→ 8秒経過：最初から再生します！"));
			CurrentBattleVolume = SilentVolume;
			BattleAudioComp->SetVolumeMultiplier(SilentVolume);
			BattleAudioComp->SetPaused(false);
			BattleAudioComp->Stop();
			BattleAudioComp->Play(0.0f);
		}
	}
	else
	{
		// 【戦闘終了】
		LastCombatEndTime = GetWorld()->GetTimeSeconds();

		TargetFieldVolume = 1.0f;
		TargetBattleVolume = SilentVolume;

		FieldAudioComp->SetPaused(false);
	}
}

void UMusicControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!FieldAudioComp || !BattleAudioComp) return;

	float FadeSpeed = 1.0f / FadeDuration;

	// --- フィールド曲のフェード ---
	if (CurrentFieldVolume != TargetFieldVolume)
	{
		CurrentFieldVolume = FMath::FInterpConstantTo(CurrentFieldVolume, TargetFieldVolume, DeltaTime, FadeSpeed);
		FieldAudioComp->SetVolumeMultiplier(CurrentFieldVolume);
	}
	// 音量が下限に達し、かつ「再生中」の時だけ1回だけポーズする
	if (CurrentFieldVolume <= SilentVolume && TargetFieldVolume <= SilentVolume && FieldAudioComp->IsPlaying())
	{
		FieldAudioComp->SetPaused(true);
	}

	// --- 戦闘曲のフェード ---
	if (CurrentBattleVolume != TargetBattleVolume)
	{
		CurrentBattleVolume = FMath::FInterpConstantTo(CurrentBattleVolume, TargetBattleVolume, DeltaTime, FadeSpeed);
		BattleAudioComp->SetVolumeMultiplier(CurrentBattleVolume);
	}
	// 音量が下限に達し、かつ「再生中」の時だけポーズする
	if (CurrentBattleVolume <= SilentVolume && TargetBattleVolume <= SilentVolume && BattleAudioComp->IsPlaying())
	{
		BattleAudioComp->SetPaused(true);
	}
}