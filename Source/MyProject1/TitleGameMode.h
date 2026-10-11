#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TitleGameMode.generated.h"

class UUserWidget;

/**
 * タイトルマップ専用のGameMode。プレイヤーキャラは出さず、TitleMenuClassのWidgetを表示するだけ。
 * 使い方：これを親にしたBP（BP_TitleGameMode）を作り、TitleMenuClassにWBP_TitleMenuを指定して、
 * タイトルマップのWorld Settings → GameMode Overrideに設定する。
 */
UCLASS()
class MYPROJECT1_API ATitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATitleGameMode();

protected:
	virtual void BeginPlay() override;

	/** タイトル開始時に表示するWidget（WBP_TitleMenu） */
	UPROPERTY(EditDefaultsOnly, Category = "Title")
	TSubclassOf<UUserWidget> TitleMenuClass;
};
