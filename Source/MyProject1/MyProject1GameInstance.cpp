#include "MyProject1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "MyProject1Character.h"

// ----------------------------------------------------
// 1. ワープの「要求」と「暗転の開始」
// ----------------------------------------------------
void UMyProject1GameInstance::RequestWarp(FName WarpID, ACharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !WarpDataTable || WarpID.IsNone()) return;

	FWarpDestination* WarpData = WarpDataTable->FindRow<FWarpDestination>(WarpID, TEXT("WarpContext"));
	if (!WarpData)
	{
		UE_LOG(LogTemp, Warning, TEXT("WarpID: %s not found!"), *WarpID.ToString());
		return;
	}

	AMyProject1Character* MyChar = Cast<AMyProject1Character>(PlayerCharacter);
	if (MyChar && !WarpData->RequiredFlag.IsNone())
	{
		if (!MyChar->HasFlag(WarpData->RequiredFlag))
		{
			MyChar->OnReceiveLogMessage(TEXT("A mysterious force prevents you from advancing..."), ELogMessageType::System);
			return;
		}
	}

	// --- ★変更：ここから下を「暗転開始」の処理に書き換えます ---

	// 後で移動できるように情報を予約しておく
	ReservedWarpID = WarpID;
	ReservedPlayer = PlayerCharacter;

	// キャラクターの入力を禁止する（動けなくする）
	if (APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController()))
	{
		PlayerCharacter->DisableInput(PC);
	}

	// UI（Widget）へ「暗転開始アニメーションを再生して！」と合図を送る
	OnWarpFadeOutRequested.Broadcast();
}

// ----------------------------------------------------
// 2. 暗転が終わった後に呼ばれる「実際の移動」
// ----------------------------------------------------
void UMyProject1GameInstance::ExecuteWarpProcess()
{
	// 予約された情報がなければ何もしない
	if (!ReservedPlayer.IsValid() || ReservedWarpID.IsNone() || !WarpDataTable) return;

	FWarpDestination* WarpData = WarpDataTable->FindRow<FWarpDestination>(ReservedWarpID, TEXT("WarpContext"));
	if (!WarpData) return;

	FName CurrentLevelName = FName(*UGameplayStatics::GetCurrentLevelName(GetWorld()));
	FName TargetLevelName = WarpData->TargetLevelName;

	bool bIsSameLevel = TargetLevelName.IsNone() || (TargetLevelName == CurrentLevelName);

	if (bIsSameLevel)
	{
		// 【パターンA】同じマップの場合：暗転した画面の裏で座標を移動させる
		if (ReservedPlayer.IsValid())
		{
			ReservedPlayer->SetActorTransform(WarpData->DestinationTransform);

			// カメラの向きも合わせる
			if (APlayerController* PC = Cast<APlayerController>(ReservedPlayer->GetController()))
			{
				PC->SetControlRotation(WarpData->DestinationTransform.GetRotation().Rotator());
			}
		}
		// ※同じマップの場合は、この直後にUI側で「フェードイン」を開始させる仕組みが必要です。
	}
	else
	{
		// 【パターンB】別のマップの場合：座標を記憶して、マップを開く
		bHasPendingWarp = true;
		PendingWarpTransform = WarpData->DestinationTransform;

		UGameplayStatics::OpenLevel(GetWorld(), TargetLevelName);
	}

	// 予約情報をリセットしておく
	ReservedWarpID = NAME_None;
	ReservedPlayer.Reset();
}

// ----------------------------------------------------
// 3. レベル移動後の座標適用（既存のまま）[cite: 10]
// ----------------------------------------------------
void UMyProject1GameInstance::ApplyPendingWarp(ACharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !bHasPendingWarp) return;

	PlayerCharacter->SetActorTransform(PendingWarpTransform);

	if (APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController()))
	{
		PC->SetControlRotation(PendingWarpTransform.GetRotation().Rotator());
		PC->SetViewTarget(PlayerCharacter);
	}

	bHasPendingWarp = false;
}