#include "MyProject1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "MyProject1Character.h"


void UMyProject1GameInstance::Init()
{
	Super::Init();

	// 現実の1秒ごとに UpdateInGameTime を呼ぶタイマーをセット
	GetTimerManager().SetTimer(TimeUpdateTimerHandle, this, &UMyProject1GameInstance::UpdateInGameTime, RealSecondsPerGameMinute, true);
}

void UMyProject1GameInstance::UpdateInGameTime()
{
	// 例：現実の1秒で、ゲーム内の1分が進む設計（倍率は自由に調整可）
	CurrentTimeInMinutes += 1;

	// 24時間（1440分）を超えたら0に戻す（翌日）
	if (CurrentTimeInMinutes >= 1440)
	{
		CurrentTimeInMinutes = 0;
		AdvanceDay();
	}

	// 「時」と「分」を計算
	int32 Hour = CurrentTimeInMinutes / 60;
	int32 Minute = CurrentTimeInMinutes % 60;

	// UIに向けて「時間が変わったよ！」とお知らせする
	if (OnInGameTimeChanged.IsBound())
	{
		OnInGameTimeChanged.Broadcast(CurrentYear, CurrentMonth, CurrentDay, Hour, Minute);
	}
}

// 日付を1日進める処理
void UMyProject1GameInstance::AdvanceDay()
{
	CurrentDay++;
	TotalElapsedDays++;

	// その月の最終日を超えたら、翌月の1日にする
	if (CurrentDay > GetDaysInMonth(CurrentYear, CurrentMonth))
	{
		CurrentDay = 1;
		CurrentMonth++;

		// 12月を超えたら、翌年の1月にする
		if (CurrentMonth > 12)
		{
			CurrentMonth = 1;
			CurrentYear++;
		}
	}
}

// 指定した「年・月」の日数を計算する処理
int32 UMyProject1GameInstance::GetDaysInMonth(int32 Year, int32 Month)
{
	// 1月〜12月までの各月の日数リスト（0番目は空っぽ）
	static const int32 DaysPerMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if (Month == 2)
	{
		// 2月の場合：閏年（うるうどし）の計算を行う
		bool bIsLeapYear = (Year % 4 == 0 && Year % 100 != 0) || (Year % 400 == 0);
		return bIsLeapYear ? 29 : 28;
	}

	// 正しい月の範囲なら配列から日数を返し、エラー値なら安全のため31を返す
	return (Month >= 1 && Month <= 12) ? DaysPerMonth[Month] : 31;
}

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

