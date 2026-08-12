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

	// RoomMusicVolumeに入った時に呼ばれる。部屋専用BGMへクロスフェードする
	void EnterRoomMusic(TSoftObjectPtr<USoundBase> NewRoomMusic);

	// RoomMusicVolumeから出た時に呼ばれる。フィールドBGMへ戻す
	void ExitRoomMusic();

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

	// 再入場時に同じ部屋なら再クロスフェードしないようにするための比較用
	TSoftObjectPtr<USoundBase> CurrentRoomMusic;

	// --- フェード計算用のボリューム管理変数 ---
	float TargetFieldVolume = 1.0f;
	float TargetBattleVolume = 0.0f;
	float TargetRoomVolume = 0.0f;
	float CurrentFieldVolume = 1.0f;
	float CurrentBattleVolume = 0.0f;
	float CurrentRoomVolume = 0.0f;
};