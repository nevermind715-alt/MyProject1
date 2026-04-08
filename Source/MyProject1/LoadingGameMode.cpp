#include "LoadingGameMode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "MyProject1GameInstance.h" // ★追加：GameInstanceの機能を使うため

// 日本語文字化け対策
#pragma execution_character_set("utf-8")

ALoadingGameMode::ALoadingGameMode()
{
	// ゲーム開始を保留にする
	bDelayedStart = true;
	WaitTime = 0.0f;
}

bool ALoadingGameMode::ReadyToStartMatch_Implementation()
{
	if (!Super::ReadyToStartMatch_Implementation()) return false;

	WaitTime += GetWorld()->GetDeltaSeconds();
	if (WaitTime < 1.0f) return false;

	// ワールドの読み込み完了チェック
	if (GetWorld()->AreAlwaysLoadedLevelsLoaded())
	{
		GEngine->BlockTillLevelStreamingCompleted(GetWorld());
		return true; // 準備完了！これで StartMatch() が発動します
	}

	return false;
}

void ALoadingGameMode::StartMatch()
{
	// まずはエンジン本来の開始処理
	Super::StartMatch();

	// GameInstanceからワープ先の記憶を引っ張ってくる
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC)
		{
			// 1. 待機中に被らされていた「幽霊カメラ（SpectatorPawn）」を強制的に消し去る
			if (PC->GetPawn() != nullptr)
			{
				PC->GetPawn()->Destroy();
			}

			// 2. ★超重要★ コントローラーの状態を「観戦」から「プレイ中」に強制で切り替える
			PC->bPlayerIsWaiting = false;
			PC->ChangeState(NAME_Playing);

			// 3. ワープ予定がある場合、PlayerStartを無視して「直接その場に」肉体をスポーンさせる！
			if (GameInst && GameInst->bHasPendingWarp && DefaultPawnClass != nullptr)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				// 直接肉体を作る
				APawn* NewPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, GameInst->PendingWarpTransform, SpawnParams);
				if (NewPawn)
				{
					// 魂を乗り移らせる
					PC->Possess(NewPawn);
				}
			}
			else
			{
				// ワープじゃない場合（普通にゲーム開始した場合）はいつものスポーン
				RestartPlayer(PC);
			}
		}
	}
}