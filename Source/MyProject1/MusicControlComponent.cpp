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

	RoomAudioComp = NewObject<UAudioComponent>(GetOwner());
	RoomAudioComp->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	RoomAudioComp->bAutoDestroy = false;
	RoomAudioComp->RegisterComponent();
	RoomAudioComp->bAutoActivate = false;
	RoomAudioComp->bIsUISound = true;

	AMusicManagerActor* LevelMusicManager = nullptr;
	for (TActorIterator<AMusicManagerActor> It(GetWorld()); It; ++It)
	{
		LevelMusicManager = *It;
		break;
	}

	if (LevelMusicManager)
	{
		if (!LevelMusicManager->FieldMusic.IsNull())
		{
			USoundBase* LoadedFieldMusic = LevelMusicManager->FieldMusic.LoadSynchronous();
			FieldAudioComp->SetSound(LoadedFieldMusic);
			FieldAudioComp->Play();
			CurrentFieldVolume = 1.0f;
		}

		if (!LevelMusicManager->BattleMusic.IsNull())
		{
			USoundBase* LoadedBattleMusic = LevelMusicManager->BattleMusic.LoadSynchronous();
			BattleAudioComp->SetSound(LoadedBattleMusic);
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
		// 【戦闘開始】戦闘曲を目標1.0に。フィールド曲はRefreshFieldTargetで無音化される
		RefreshFieldTarget();
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

		TargetBattleVolume = SilentVolume;
		RefreshFieldTarget();

		if (!bInRoom)
		{
			FieldAudioComp->SetPaused(false);
		}
	}
}

void UMusicControlComponent::RefreshFieldTarget()
{
	// 戦闘中または部屋の中にいる間は、フィールド曲を無音にしておく
	TargetFieldVolume = (bIsCombatMusicPlaying || bInRoom) ? SilentVolume : 1.0f;
}

void UMusicControlComponent::EnterRoomMusic(TSoftObjectPtr<USoundBase> NewRoomMusic)
{
	if (!RoomAudioComp) return;

	// 同じ部屋に入り直した場合（同一Volume内で再Overlapした場合など）は何もしない
	if (bInRoom && CurrentRoomMusic == NewRoomMusic) return;

	bInRoom = true;
	CurrentRoomMusic = NewRoomMusic;
	RefreshFieldTarget();

	if (!NewRoomMusic.IsNull())
	{
		USoundBase* LoadedRoomMusic = NewRoomMusic.LoadSynchronous();
		RoomAudioComp->SetSound(LoadedRoomMusic);
		RoomAudioComp->SetVolumeMultiplier(SilentVolume);
		CurrentRoomVolume = SilentVolume;
		RoomAudioComp->SetPaused(false);
		RoomAudioComp->Stop();
		RoomAudioComp->Play(0.0f);
		TargetRoomVolume = 1.0f;
	}
	else
	{
		// 部屋にBGMが設定されていない場合は、フィールド曲を無音化するだけ（無音の部屋）
		TargetRoomVolume = SilentVolume;
	}
}

void UMusicControlComponent::ExitRoomMusic()
{
	if (!RoomAudioComp) return;
	if (!bInRoom) return;

	bInRoom = false;
	CurrentRoomMusic = nullptr;
	TargetRoomVolume = SilentVolume;
	RefreshFieldTarget();

	if (!bIsCombatMusicPlaying)
	{
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

	// --- 部屋BGMのフェード ---
	if (!RoomAudioComp) return;

	if (CurrentRoomVolume != TargetRoomVolume)
	{
		CurrentRoomVolume = FMath::FInterpConstantTo(CurrentRoomVolume, TargetRoomVolume, DeltaTime, FadeSpeed);
		RoomAudioComp->SetVolumeMultiplier(CurrentRoomVolume);
	}
	if (CurrentRoomVolume <= SilentVolume && TargetRoomVolume <= SilentVolume && RoomAudioComp->IsPlaying())
	{
		RoomAudioComp->SetPaused(true);
	}
}

void UMusicControlComponent::PlayDeathMusic()
{
	// 1. 現在の戦闘曲とフィールド曲の両方をフェードアウト状態（無音ターゲット）にする
	TargetBattleVolume = SilentVolume;
	TargetFieldVolume = SilentVolume;
	bIsCombatMusicPlaying = false; // 状態をリセット

	if (!DeathMusic.IsNull())
	{
		USoundBase* LoadedDeathMusic = DeathMusic.LoadSynchronous();
		UAudioComponent* DeathAudio = UGameplayStatics::SpawnSound2D(this, LoadedDeathMusic);
		if (DeathAudio)
		{
			DeathAudio->FadeIn(FadeDuration, 1.0f);

			float MusicDuration = LoadedDeathMusic->GetDuration();
			GetWorld()->GetTimerManager().SetTimer(DeathMusicTimerHandle, this, &UMusicControlComponent::OnDeathMusicFinished, MusicDuration, false);
		}
	}
	else
	{
		// 死亡BGMがセットされていない場合はすぐにフィールド曲へ戻す
		OnDeathMusicFinished();
	}
}

void UMusicControlComponent::OnDeathMusicFinished()
{
	// SetCombatMusicActive(false) を使うと「すでにfalse」と判定されてスキップされてしまうため、
	// ここで直接フィールド曲をフェードインさせるように設定を上書きします。

	bIsCombatMusicPlaying = false;

	// フィールド曲の目標音量を MAX (1.0) に戻す
	TargetFieldVolume = 1.0f;

	// 戦闘曲は確実に無音にする
	TargetBattleVolume = SilentVolume;

	// フィールド曲が一時停止(Pause)されていたら、再生を再開する
	if (FieldAudioComp)
	{
		FieldAudioComp->SetPaused(false);
	}
}