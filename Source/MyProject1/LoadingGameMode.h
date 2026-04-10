#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h" // 完全版のGameMode
#include "LoadingGameMode.generated.h"

UCLASS()
class MYPROJECT1_API ALoadingGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ALoadingGameMode();

protected:
	// ロードが終わったか判定する関数
	virtual bool ReadyToStartMatch_Implementation() override;

	// ★追加：ロードが完了し、いよいよゲームが始まる瞬間に呼ばれる関数！
	virtual void StartMatch() override;

private:
	float WaitTime;
};