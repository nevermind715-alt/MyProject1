#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MusicControlComponent.generated.h"

class UAudioComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UMusicControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMusicControlComponent();

	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Music Control")
	void SetCombatMusicActive(bool bIsCombat);

	// 死亡時のBGM
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Control")
	TSoftObjectPtr<USoundBase> DeathMusic;

	// 死亡BGMを再生する関数
	UFUNCTION(BlueprintCallable, Category = "Music Control")
	void PlayDeathMusic();

	// フェードにかかる時間（秒）。BPから自由に変更可能です
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Control")
	float FadeDuration = 1.5f;

	// RoomMusicVolumeに入った時に呼ばれる。部屋専用BGMへクロスフェードする(VolumeScaleは部屋ごとの音量オーバーライド、1.0が基準)
	void EnterRoomMusic(TSoftObjectPtr<USoundBase> NewRoomMusic, float VolumeScale = 1.0f);

	// RoomMusicVolumeから出た時に呼ばれる。フィールドBGMへ戻す
	void ExitRoomMusic();

	// DT_AnimEvents::EventBGM等、RoomMusicVolumeとは無関係に一時的にBGMを上書きしたい時に呼ぶ。
	// EnterRoomMusicと同様に動作するが、呼び出し時点の部屋BGM状態（RoomMusicVolume内にいたかどうか）を
	// 記憶しておき、ExitOverrideMusicでその状態へ正しく戻せるようにする
	void EnterOverrideMusic(TSoftObjectPtr<USoundBase> OverrideMusic, float VolumeScale = 1.0f);

	// EnterOverrideMusicで一時的に上書きしたBGMを終える。EnterOverrideMusic呼び出し時点で
	// RoomMusicVolume内にいた場合はその部屋のBGMへ、外にいた場合はExitRoomMusicと同じくフィールドBGMへ戻す
	void ExitOverrideMusic();

protected:
	virtual void BeginPlay() override;

private:
	// 現在のCombat/Room状態から、フィールド曲が鳴るべき目標音量を再計算する
	void RefreshFieldTarget();
	// タイマーハンドルと終了時に呼ばれる関数
	FTimerHandle DeathMusicTimerHandle;

	UFUNCTION()
	void OnDeathMusicFinished();

	UPROPERTY()
	UAudioComponent* FieldAudioComp;

	UPROPERTY()
	UAudioComponent* BattleAudioComp;

	UPROPERTY()
	UAudioComponent* RoomAudioComp;

	bool bIsCombatMusicPlaying = false;
	double LastCombatEndTime = -100.0;
	const float CombatMusicResumeThreshold = 8.0f;

	// 現在部屋の中にいるかどうか。Combatと合わせてフィールド曲の目標音量を決める
	bool bInRoom = false;

	// 再入場時に同じ部屋なら再クロスフェードしないようにするための比較用。
	// 部屋を出た後も「直前にいた部屋の曲」として保持し続け、続きから再生の判定に使う
	TSoftObjectPtr<USoundBase> CurrentRoomMusic;

	// 部屋から出た時刻。この時刻からRoomMusicResumeThreshold以内に同じ曲の部屋へ戻った場合は続きから再生する
	double LastRoomExitTime = -100.0;
	const float RoomMusicResumeThreshold = 8.0f;

	// EnterOverrideMusicを呼ぶ直前の部屋BGM状態（ExitOverrideMusicで正しい状態へ戻すために記憶しておく）
	bool bHadRoomBeforeOverride = false;
	TSoftObjectPtr<USoundBase> RoomMusicBeforeOverride;
	float RoomVolumeBeforeOverride = 1.0f;

	// --- フェード計算用のボリューム管理変数 ---
	float TargetFieldVolume = 1.0f;
	float TargetBattleVolume = 0.0f;
	float TargetRoomVolume = 0.0f;
	float CurrentFieldVolume = 1.0f;
	float CurrentBattleVolume = 0.0f;
	float CurrentRoomVolume = 0.0f;
};