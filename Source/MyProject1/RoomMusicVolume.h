#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomMusicVolume.generated.h"

class UBoxComponent;

// 箱型のコリジョンでレベル上の「部屋」を囲み、プレイヤーがその中にいる間だけ
// 専用のBGMを再生するアクタ。見た目はTrigger Volumeと同じくワイヤーフレームの箱。
UCLASS()
class MYPROJECT1_API ARoomMusicVolume : public AActor
{
	GENERATED_BODY()

public:
	ARoomMusicVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* BoxComp;

	// この部屋専用のBGM。未設定の場合はフィールドBGMがそのまま流れ続ける
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Music")
	TSoftObjectPtr<USoundBase> RoomMusic;

	// この部屋のBGM音量(1.0が基準)。部屋ごとに音量を個別調整したい場合に使用する
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Music", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float Volume = 1.0f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
