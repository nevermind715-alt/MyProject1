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

	UFUNCTION(BlueprintCallable, Category = "Music Control")
	void SetCombatMusicActive(bool bIsCombat);

	// 毎フレームのフェード処理を行うための関数
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// フェードにかかる時間（秒）。BPから自由に変更可能です
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Control")
	float FadeDuration = 1.5f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UAudioComponent* FieldAudioComp;

	UPROPERTY()
	UAudioComponent* BattleAudioComp;

	bool bIsCombatMusicPlaying = false;
	double LastCombatEndTime = -100.0;
	const float CombatMusicResumeThreshold = 8.0f;

	// --- フェード計算用のボリューム管理変数 ---
	float TargetFieldVolume = 1.0f;
	float TargetBattleVolume = 0.0f;
	float CurrentFieldVolume = 1.0f;
	float CurrentBattleVolume = 0.0f;
};