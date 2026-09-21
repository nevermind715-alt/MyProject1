#include "MyProject1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "MyProject1Character.h"
#include "MyProject1SaveGame.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "SkinOverlayComponent.h"
#include "GameFramework/PlayerController.h"
#include "WallWarpLink.h"
#include "GameplayActionLibrary.h"
#include "RpgCharacterInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "MusicControlComponent.h"


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

// 待機/睡眠などで時間を一気に進める（分単位）
void UMyProject1GameInstance::AdvanceTimeBy(int32 MinutesToAdd)
{
	if (MinutesToAdd <= 0)
	{
		return;
	}

	CurrentTimeInMinutes += MinutesToAdd;

	// 日をまたぐ分だけAdvanceDayを個別に呼ぶ（複数日またぐ待機でも1日ずつ正しく進む）
	while (CurrentTimeInMinutes >= 1440)
	{
		CurrentTimeInMinutes -= 1440;
		AdvanceDay();
	}

	int32 Hour = CurrentTimeInMinutes / 60;
	int32 Minute = CurrentTimeInMinutes % 60;

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

	if (OnDayChangedDelegate.IsBound())
	{
		OnDayChangedDelegate.Broadcast();
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
void UMyProject1GameInstance::RequestWarp(FName WarpID, ACharacter* PlayerCharacter, bool bBypassRequiredFlag)
{
	if (!PlayerCharacter) { UE_LOG(LogTemp, Warning, TEXT("RequestWarp: PlayerCharacter is null")); return; }
	if (!WarpDataTable) { UE_LOG(LogTemp, Warning, TEXT("RequestWarp: WarpDataTable is not set on GameInstance")); return; }
	if (WarpID.IsNone()) { UE_LOG(LogTemp, Warning, TEXT("RequestWarp: WarpID is None")); return; }

	FWarpDestination* WarpData = WarpDataTable->FindRow<FWarpDestination>(WarpID, TEXT("WarpContext"));
	if (!WarpData)
	{
		UE_LOG(LogTemp, Warning, TEXT("WarpID: %s not found!"), *WarpID.ToString());
		return;
	}

	AMyProject1Character* MyChar = Cast<AMyProject1Character>(PlayerCharacter);
	if (!bBypassRequiredFlag && MyChar && !WarpData->RequiredFlag.IsNone())
	{
		if (!MyChar->HasFlag(WarpData->RequiredFlag))
		{
			const FString Msg = WarpData->RequiredFlagMessage.IsEmpty() ? TEXT("何か目に見えない力に阻まれて、先に進めない…。") : WarpData->RequiredFlagMessage;
			MyChar->OnReceiveLogMessage(Msg, ELogMessageType::System);
			return;
		}
	}

	// --- ★変更：ここから下を「暗転開始」の処理に書き換えます ---

	// 後で移動できるように情報を予約しておく
	ReservedWarpID = WarpID;
	ReservedPlayer = PlayerCharacter;

	BeginWarpFade(PlayerCharacter);
}

// ----------------------------------------------------
// 1.5. ワープを伴わない「暗転を挟んだフラグ付与」の要求（NPCの表示切替など向け）
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenGrantFlag(FName FlagName, AMyProject1Character* TargetCharacter)
{
	if (FlagName.IsNone() || !TargetCharacter) return;

	ReservedFlagToGrant = FlagName;
	ReservedFlagGrantTarget = TargetCharacter;

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.5.5. ワープを伴わない「暗転を挟んだフラグ消去」の要求（NPCの表示切替など向け）
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenRemoveFlag(FName FlagName, AMyProject1Character* TargetCharacter)
{
	if (FlagName.IsNone() || !TargetCharacter) return;

	ReservedFlagToRemove = FlagName;
	ReservedFlagRemoveTarget = TargetCharacter;

	// 同じ選択肢のActionType=Warp（またはWallWarp）が既にこのフレームで暗転を予約済みなら、
	// そちらの暗転にそのまま相乗りする（二重にOnWarpFadeOutRequestedを鳴らして暗転を壊さないため）。
	// フラグの消去自体はExecuteWarpProcessの先頭で必ず反映されるので、ここでreturnしても消え忘れない
	if (!ReservedWarpID.IsNone() || ReservedWallWarpLink.IsValid())
	{
		return;
	}

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.5.6. 待機/睡眠による「暗転を挟んだ時間スキップ」の要求
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenAdvanceTime(int32 MinutesToAdd, ACharacter* TargetCharacter, bool bIsSleep)
{
	if (MinutesToAdd <= 0 || !TargetCharacter) return;

	ReservedTimeSkipMinutes = MinutesToAdd;
	ReservedTimeSkipCharacter = TargetCharacter;
	ReservedTimeSkipIsSleep = bIsSleep;

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.6. WallWarpLink用の「暗転を挟んだ軽量ワープ」の要求
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenWallWarp(AWallWarpLink* SourceLink, ACharacter* TargetCharacter)
{
	if (!SourceLink || !TargetCharacter) return;

	ReservedWallWarpLink = SourceLink;
	ReservedWallWarpCharacter = TargetCharacter;

	BeginWarpFade(TargetCharacter);
}

// ----------------------------------------------------
// 1.7. 暗転演出の共通処理（入力停止→暗転タイマー予約→UIへ合図）
// ----------------------------------------------------
void UMyProject1GameInstance::BeginWarpFade(ACharacter* TargetCharacter)
{
	if (!TargetCharacter) return;

	if (APlayerController* PC = Cast<APlayerController>(TargetCharacter->GetController()))
	{
		TargetCharacter->DisableInput(PC);
	}
	InputDisabledCharacter = TargetCharacter;

	// 同じフレームで複数のRequest系が呼ばれても（フラグ消去の相乗り等）、暗転タイマーは1本だけ動かす
	if (!GetTimerManager().IsTimerActive(WarpFadeOutTimerHandle))
	{
		GetTimerManager().SetTimer(WarpFadeOutTimerHandle, this,
			&UMyProject1GameInstance::HandleWarpFadeOutComplete, WarpFadeOutDuration, false);
	}

	// UI（Widget）へ「暗転開始アニメーションを再生して！」と合図を送る
	OnWarpFadeOutRequested.Broadcast();
}

// ----------------------------------------------------
// 1.8. 暗転が終わった（＝画面が真っ暗になった）タイミングで自動的に呼ばれる
// ----------------------------------------------------
void UMyProject1GameInstance::HandleWarpFadeOutComplete()
{
	ExecuteWarpProcess();

	GetTimerManager().SetTimer(WarpFadeInTimerHandle, this,
		&UMyProject1GameInstance::HandleWarpFadeInComplete, WarpFadeInDuration, false);
}

// ----------------------------------------------------
// 1.9. 明転が終わったタイミングで自動的に呼ばれ、入力を戻す
// ----------------------------------------------------
void UMyProject1GameInstance::HandleWarpFadeInComplete()
{
	if (ACharacter* Character = InputDisabledCharacter.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			Character->EnableInput(PC);
		}
	}
	InputDisabledCharacter.Reset();

	BeginAnimEventSequenceIfNeeded();
}

// ----------------------------------------------------
// 2. 暗転が終わった後に呼ばれる「実際の移動」
// ----------------------------------------------------
void UMyProject1GameInstance::ExecuteWarpProcess()
{
	// フラグ消去の予約は、Warp/WallWarp等の他の予約と同じ暗転に相乗りしている可能性があるため、
	// ここではreturnせず必ず先に消化してから、下の各分岐（Warp本来の移動など）へ続ける
	if (!ReservedFlagToRemove.IsNone())
	{
		if (AMyProject1Character* Character = ReservedFlagRemoveTarget.Get())
		{
			Character->RemoveFlag(ReservedFlagToRemove);
		}

		ReservedFlagToRemove = NAME_None;
		ReservedFlagRemoveTarget.Reset();
	}

	// 待機/睡眠による時間スキップの予約があれば、こちらで完結させる（RequestFadeThenAdvanceTime用）
	if (ReservedTimeSkipMinutes > 0)
	{
		AdvanceTimeBy(ReservedTimeSkipMinutes);

		// HandleFatigueTickは現実時間の経過にしか反応しないため、待機/睡眠でジャンプした分の疲労度は
		// ここで明示的に反映する（そうしないと待機による時間経過分の疲労上昇が抜け落ちる）
		if (AMyProject1Character* SkippedCharacter = Cast<AMyProject1Character>(ReservedTimeSkipCharacter.Get()))
		{
			SkippedCharacter->ApplyFatigueForSkippedMinutes(ReservedTimeSkipMinutes, ReservedTimeSkipIsSleep);
		}

		ReservedTimeSkipMinutes = 0;
		ReservedTimeSkipCharacter.Reset();
		ReservedTimeSkipIsSleep = false;
		return;
	}

	// ワープの予約より先に、フラグ付与だけの予約がないか確認する（RequestFadeThenGrantFlag経由の場合はこちらで完結させる）
	if (!ReservedFlagToGrant.IsNone())
	{
		if (AMyProject1Character* Character = ReservedFlagGrantTarget.Get())
		{
			Character->AddFlag(ReservedFlagToGrant);
		}

		ReservedFlagToGrant = NAME_None;
		ReservedFlagGrantTarget.Reset();
		return;
	}

	// WallWarpLink経由の予約があれば、こちらで完結させる（RequestFadeThenWallWarp用）
	if (ReservedWallWarpLink.IsValid() && ReservedWallWarpCharacter.IsValid())
	{
		ReservedWallWarpLink->ExecuteWarpNow(ReservedWallWarpCharacter.Get());

		ReservedWallWarpLink.Reset();
		ReservedWallWarpCharacter.Reset();
		return;
	}

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

			// イベント分岐で敗北→同一レベル内の施設へワープしてきた場合、bIsDeadのまま行動不能にならないよう復帰させる
			if (bHasActiveEvent)
			{
				if (AMyProject1Character* MyChar = Cast<AMyProject1Character>(ReservedPlayer.Get()))
				{
					MyChar->Revive(1.0f);
				}
			}
		}
		// 明転と入力復帰は、この関数の呼び出し元であるHandleWarpFadeOutCompleteが
		// WarpFadeInDuration秒後に自動的に処理する（同じマップ/別マップどちらの場合も共通）
	}
	else
	{
		// 【パターンB】別のマップの場合：座標を記憶して、マップを開く
		bHasPendingWarp = true;
		PendingWarpTransform = WarpData->DestinationTransform;

		// クエスト・所持品・装備・ステータス（称号フラグ含む）は、レベル移動でキャラクターが
		// 再生成されると初期値に戻ってしまう。消える前の状態を一時スナップショットとして記憶しておき、
		// 新しいキャラクターのBeginPlay（ApplyPendingCharacterLoad）に読み戻させる。
		if (AMyProject1Character* MyChar = Cast<AMyProject1Character>(ReservedPlayer.Get()))
		{
			if (UMyProject1SaveGame* Snapshot = CapturePlayerStateSnapshot(MyChar))
			{
				PendingLoadSaveGame = Snapshot;
			}
		}

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

// ----------------------------------------------------
// デバッグワープメニュー等がBP側で一覧UIを組み立てるためのデータ取得
// ----------------------------------------------------
TArray<FWarpDestinationInfo> UMyProject1GameInstance::GetAllWarpDestinations() const
{
	TArray<FWarpDestinationInfo> Result;
	if (!WarpDataTable) return Result;

	for (const FName& RowName : WarpDataTable->GetRowNames())
	{
		const FWarpDestination* Row = WarpDataTable->FindRow<FWarpDestination>(RowName, TEXT("GetAllWarpDestinations"));
		if (!Row) continue;

		FWarpDestinationInfo Info;
		Info.WarpID = RowName;
		Info.DisplayName = Row->DisplayName.IsEmpty() ? FText::FromName(RowName) : Row->DisplayName;
		Result.Add(Info);
	}
	return Result;
}

// ----------------------------------------------------
// セーブ/ロード
// ----------------------------------------------------

const FString UMyProject1GameInstance::AutoSaveSlotName = TEXT("AutoSave");

FString UMyProject1GameInstance::GetManualSaveSlotName(int32 SlotIndex)
{
	// 呼び出し側のミスで範囲外が来ても安全なスロット名になるようクランプする
	SlotIndex = FMath::Clamp(SlotIndex, 1, NumManualSaveSlots);
	return FString::Printf(TEXT("SaveSlot%d"), SlotIndex);
}

UMyProject1SaveGame* UMyProject1GameInstance::CapturePlayerStateSnapshot(AMyProject1Character* Character)
{
	if (!Character) return nullptr;

	UMyProject1SaveGame* SaveObj = Cast<UMyProject1SaveGame>(UGameplayStatics::CreateSaveGameObject(UMyProject1SaveGame::StaticClass()));
	if (!SaveObj) return nullptr;

	// 位置と所属レベル
	SaveObj->PlayerLevelName = FName(*UGameplayStatics::GetCurrentLevelName(GetWorld()));
	SaveObj->PlayerTransform = Character->GetActorTransform();

	// ステータス（称号フラグ=UnlockedFlagsもMyStatsに含まれる）
	SaveObj->PlayerStats = Character->MyStats;

	// 装備
	SaveObj->EquippedItems = Character->CurrentEquippedItems;

	// 所持品
	if (UInventoryComponent* Inv = Character->FindComponentByClass<UInventoryComponent>())
	{
		SaveObj->InventoryContent = Inv->InventoryContent;
		SaveObj->Gil = Inv->Gil;
		SaveObj->ObtainedRareItemIDs = Inv->ObtainedRareItemIDs;
	}

	// クエスト進行
	if (UQuestComponent* Quest = Character->GetQuestComponent())
	{
		SaveObj->ActiveQuests = Quest->ActiveQuests;
		SaveObj->CompletedQuests = Quest->CompletedQuests;
		SaveObj->EverCompletedQuestIDs = Quest->EverCompletedQuestIDs;
	}

	// 傷・タトゥー・ピアス・病気
	if (USkinOverlayComponent* Skin = Character->FindComponentByClass<USkinOverlayComponent>())
	{
		SaveObj->ActiveTattoos = Skin->GetActiveTattoos();
		SaveObj->ActiveScars = Skin->GetActiveScars();
		SaveObj->ActivePiercings = Skin->GetActivePiercings();
		SaveObj->ActiveDiseases = Skin->GetActiveDiseases();
	}

	// 暦・時間（GameInstance常駐データ）
	SaveObj->CurrentTimeInMinutes = CurrentTimeInMinutes;
	SaveObj->CurrentYear = CurrentYear;
	SaveObj->CurrentMonth = CurrentMonth;
	SaveObj->CurrentDay = CurrentDay;
	SaveObj->TotalElapsedDays = TotalElapsedDays;
	SaveObj->CurrentCycleState = CurrentCycleState;

	return SaveObj;
}

bool UMyProject1GameInstance::SaveCurrentGame(const FString& SlotName)
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	AMyProject1Character* Character = PC ? Cast<AMyProject1Character>(PC->GetPawn()) : nullptr;
	if (!Character) return false;

	UMyProject1SaveGame* SaveObj = CapturePlayerStateSnapshot(Character);
	if (!SaveObj) return false;

	// スロット一覧UIに出す見出し情報。ゲーム進行の復元には使わないので、ここ（明示セーブ）でのみ埋める。
	SaveObj->SavedAtRealTime = FDateTime::Now();
	SaveObj->PlayerDisplayName = SaveObj->PlayerStats.NPCName.IsEmpty() ? Character->CharacterName : SaveObj->PlayerStats.NPCName;

	return UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, 0);
}

bool UMyProject1GameInstance::AutoSaveGame()
{
	return SaveCurrentGame(AutoSaveSlotName);
}

bool UMyProject1GameInstance::LoadSavedGame(const FString& SlotName)
{
	UMyProject1SaveGame* Loaded = Cast<UMyProject1SaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Loaded) return false;

	// レベル移動後、新しいキャラクターのBeginPlayから読み込まれるまで保持しておく
	PendingLoadSaveGame = Loaded;

	// 暦・時間はGameInstance常駐データなので即座に反映
	CurrentTimeInMinutes = Loaded->CurrentTimeInMinutes;
	CurrentYear = Loaded->CurrentYear;
	CurrentMonth = Loaded->CurrentMonth;
	CurrentDay = Loaded->CurrentDay;
	TotalElapsedDays = Loaded->TotalElapsedDays;
	CurrentCycleState = Loaded->CurrentCycleState;

	// 傷・タトゥー・ピアス・病気の「箱」も先にGameInstance側へ反映しておく。
	// SkinOverlayComponent::BeginPlayがLoadOverlayStateFromGameInstance()で自動的に読みに来る。
	SavedActiveTattoos = Loaded->ActiveTattoos;
	SavedActiveScars = Loaded->ActiveScars;
	SavedActivePiercings = Loaded->ActivePiercings;
	SavedActiveDiseases = Loaded->ActiveDiseases;

	// 既存のワープ着地機構に相乗りして、保存された座標にキャラクターを出現させる
	bHasPendingWarp = true;
	PendingWarpTransform = Loaded->PlayerTransform;

	UGameplayStatics::OpenLevel(GetWorld(), Loaded->PlayerLevelName);
	return true;
}

bool UMyProject1GameInstance::DoesSaveGameExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

FSaveSlotDisplayInfo UMyProject1GameInstance::GetSaveSlotInfo(const FString& SlotName) const
{
	FSaveSlotDisplayInfo Info;
	Info.SlotName = SlotName;
	Info.bIsAutoSave = (SlotName == AutoSaveSlotName);

	// 手動スロット名（"SaveSlot3"等）から番号を取り出す。オートセーブは0のまま。
	if (!Info.bIsAutoSave && SlotName.StartsWith(TEXT("SaveSlot")))
	{
		Info.SlotIndex = FCString::Atoi(*SlotName.RightChop(8)); // "SaveSlot" は8文字
	}

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		Info.bHasData = false;
		return Info;
	}

	const UMyProject1SaveGame* Loaded = Cast<UMyProject1SaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Loaded)
	{
		// ファイルはあるが読めない（バージョン不整合など）。空き扱いにせず「壊れている」と分かる最低限だけ返す。
		Info.bHasData = false;
		return Info;
	}

	Info.bHasData = true;

	// 場所の表示名（対応表で引けなければレベル名そのまま）
	if (const FText* Mapped = LevelDisplayNameMap.Find(Loaded->PlayerLevelName))
	{
		Info.LocationName = *Mapped;
	}
	else
	{
		Info.LocationName = FText::FromName(Loaded->PlayerLevelName);
	}

	Info.PlayerName = Loaded->PlayerDisplayName;
	Info.PlayerLevel = Loaded->PlayerStats.Level;

	if (const UEnum* RankEnum = StaticEnum<EAdventurerRank>())
	{
		Info.RankText = RankEnum->GetDisplayNameTextByValue(static_cast<int64>(Loaded->PlayerStats.AdventurerRank));
	}

	Info.Gil = Loaded->Gil;
	Info.InGameYear = Loaded->CurrentYear;
	Info.InGameMonth = Loaded->CurrentMonth;
	Info.InGameDay = Loaded->CurrentDay;
	Info.TotalElapsedDays = Loaded->TotalElapsedDays;

	// 実時間のセーブ日時。旧セーブなど未設定（Ticks==0）の場合は空文字のままにしておく。
	if (Loaded->SavedAtRealTime.GetTicks() != 0)
	{
		Info.SavedAtText = Loaded->SavedAtRealTime.ToString(TEXT("%Y/%m/%d %H:%M"));
	}

	return Info;
}

TArray<FSaveSlotDisplayInfo> UMyProject1GameInstance::GetAllSaveSlotInfos() const
{
	TArray<FSaveSlotDisplayInfo> Result;
	Result.Reserve(NumManualSaveSlots + 1);

	// 先頭にオートセーブ、続いて手動スロット1〜5
	Result.Add(GetSaveSlotInfo(AutoSaveSlotName));
	for (int32 i = 1; i <= NumManualSaveSlots; ++i)
	{
		Result.Add(GetSaveSlotInfo(GetManualSaveSlotName(i)));
	}

	return Result;
}

bool UMyProject1GameInstance::ApplyPendingCharacterLoad(AMyProject1Character* Character)
{
	if (!Character || !PendingLoadSaveGame) return false;

	UMyProject1SaveGame* Loaded = PendingLoadSaveGame;

	// ステータス
	Character->MyStats = Loaded->PlayerStats;

	// 所持品
	if (UInventoryComponent* Inv = Character->FindComponentByClass<UInventoryComponent>())
	{
		Inv->InventoryContent = Loaded->InventoryContent;
		Inv->Gil = Loaded->Gil;
		Inv->ObtainedRareItemIDs = Loaded->ObtainedRareItemIDs;
		Inv->OnInventoryUpdated.Broadcast();
	}

	// クエスト進行
	if (UQuestComponent* Quest = Character->GetQuestComponent())
	{
		Quest->ActiveQuests = Loaded->ActiveQuests;
		Quest->CompletedQuests = Loaded->CompletedQuests;
		Quest->EverCompletedQuestIDs = Loaded->EverCompletedQuestIDs;
		Quest->OnQuestUpdated.Broadcast(NAME_None);
	}

	// 装備（見た目・鎖・移動制限も含めて既存のEquipItemロジックで正しく再構築させる）
	if (Character->EquipmentDataTable)
	{
		for (const TPair<EEquipmentSlot, FName>& Pair : Loaded->EquippedItems)
		{
			if (FEquipmentData* Row = Character->EquipmentDataTable->FindRow<FEquipmentData>(Pair.Value, TEXT("LoadGameEquip")))
			{
				Character->EquipItem(Pair.Value, *Row);
			}
		}
	}

	// サイクル状態はTotalElapsedDaysから再計算されるだけなので、明示的に呼んで最新化する
	Character->UpdateCycleState();

	Character->NotifyStatsChanged();

	PendingLoadSaveGame = nullptr;
	return true;
}

void UMyProject1GameInstance::AddLogHistoryEntry(const FString& Message, ELogMessageType InLogType)
{
	FLogHistoryEntry NewEntry;
	NewEntry.Message = Message;
	NewEntry.LogType = InLogType;
	LogHistory.Add(NewEntry);

	// 古いものから削除して上限件数を保つ
	while (LogHistory.Num() > MaxLogHistoryEntries)
	{
		LogHistory.RemoveAt(0);
	}
}

// ----------------------------------------------------
// イベント分岐システム
// ----------------------------------------------------
void UMyProject1GameInstance::StartEvent(FName EventID, ACharacter* PlayerCharacter, AActor* EventContextActor)
{
	if (!PlayerCharacter) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: PlayerCharacter is null")); return; }
	if (!EventDefinitionDataTable) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventDefinitionDataTable is not set on GameInstance")); return; }
	if (EventID.IsNone()) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventID is None")); return; }
	if (bHasActiveEvent) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: bHasActiveEvent is already true (ActiveEventID=%s)"), *ActiveEventID.ToString()); return; }

	FEventDefinition* Definition = EventDefinitionDataTable->FindRow<FEventDefinition>(EventID, TEXT("StartEvent"));
	if (!Definition) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventID '%s' not found in EventDefinitionDataTable"), *EventID.ToString()); return; }
	if (Definition->WarpID.IsNone()) { UE_LOG(LogTemp, Warning, TEXT("StartEvent: EventID '%s' has no WarpID set"), *EventID.ToString()); return; }

	bHasActiveEvent = true;
	ActiveEventID = EventID;
	ActiveEventPlayer = PlayerCharacter;
	ActiveEventContextActor = EventContextActor;
	bAnimEventSequenceStarted = false;

	// 成立条件に制限時間が絡む場合、ここで自動成立タイマーを仕掛けておく（Interact/AnimationSequenceの成立はこのタイマーを使わない）
	if ((Definition->ClearCondition == EEventClearCondition::TimeElapsed || Definition->ClearCondition == EEventClearCondition::Both)
		&& Definition->TimeLimitSeconds > 0.0f)
	{
		GetTimerManager().SetTimer(ActiveEventTimeLimitTimerHandle, this,
			&UMyProject1GameInstance::HandleActiveEventTimeUp, Definition->TimeLimitSeconds, false);
	}

	// RequiredFlagのチェックは施設への強制送致には意味を持たないためバイパスする
	RequestWarp(Definition->WarpID, PlayerCharacter, true);
}

void UMyProject1GameInstance::HandleActiveEventTimeUp()
{
	ResolveActiveEvent(true);
}

void UMyProject1GameInstance::ResolveActiveEvent(bool bSuccess)
{
	if (!bHasActiveEvent) return;

	GetTimerManager().ClearTimer(ActiveEventTimeLimitTimerHandle);
	GetTimerManager().ClearTimer(AnimEventStepDurationTimerHandle);
	GetTimerManager().ClearTimer(AnimEventStepTransitionTimerHandle);
	CurrentAnimEventStepIndex = INDEX_NONE;
	bAnimEventStepLooping = false;
	bAnimEventSequenceStarted = false;
	CurrentAnimEventID = NAME_None;
	AnimEventPrimaryCharacter.Reset();
	AnimEventSecondaryContextActor.Reset();
	CurrentAnimEventMontage.Reset();
	PendingAnimEventNextStepIndex = INDEX_NONE;

	FEventDefinition* Definition = EventDefinitionDataTable
		? EventDefinitionDataTable->FindRow<FEventDefinition>(ActiveEventID, TEXT("ResolveActiveEvent"))
		: nullptr;
	ACharacter* PlayerChar = ActiveEventPlayer.Get();

	if (Definition && PlayerChar)
	{
		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(PlayerChar))
		{
			const TArray<FEventAction>& Actions = bSuccess ? Definition->SuccessActions : Definition->FailureActions;
			for (const FEventAction& Action : Actions)
			{
				UGameplayActionLibrary::ExecuteAction(RpgInterface, PlayerChar, nullptr, GetWorld(),
					Action.ActionType, Action.ActionPayload, Action.ItemID, Action.ItemAmount);
				UGameplayActionLibrary::ApplyStatChange(RpgInterface, nullptr,
					Action.StatToChange, Action.StatTargetActor, Action.ExtraStatName, Action.StatChangeAmount);
			}
		}
	}

	const FName ReturnID = Definition ? Definition->ReturnWarpID : NAME_None;

	bHasActiveEvent = false;
	ActiveEventID = NAME_None;
	ActiveEventPlayer.Reset();
	ActiveEventContextActor.Reset();

	if (!ReturnID.IsNone() && PlayerChar)
	{
		RequestWarp(ReturnID, PlayerChar, true);
	}
}

// ----------------------------------------------------
// アニメーションシーケンス再生（PlayAnimSequenceEvent。イベント分岐システムとは独立。GameInstance.h参照）
// ----------------------------------------------------

// FAnimEventStep::PlayTargetに応じた再生対象キャラクターを解決する共通処理
static ACharacter* ResolveAnimEventTargetCharacter(const FAnimEventStep& Step, const TWeakObjectPtr<ACharacter>& PrimaryCharacter, const TWeakObjectPtr<AActor>& SecondaryContextActor)
{
	if (Step.PlayTarget == EStatTargetActor::NPC)
	{
		return Cast<ACharacter>(SecondaryContextActor.Get());
	}
	return PrimaryCharacter.Get();
}

// AnimSequenceDataTable内でFAnimSequenceEntry::TagがTagと一致する行を集め、その中から1つをランダムに選ぶ
// （「パンチ1」「パンチ2」のような同じカテゴリ内の複数バリエーションから抽選するための処理。Montage本体だけでなくLine/bIsPlayerLineも使うため、行そのものを返す）
static const FAnimSequenceEntry* PickRandomAnimSequenceEntryForTag(UDataTable* AnimSequenceDataTable, FName Tag)
{
	if (!AnimSequenceDataTable || Tag.IsNone()) return nullptr;

	TArray<const FAnimSequenceEntry*> Candidates;
	for (const FName& RowName : AnimSequenceDataTable->GetRowNames())
	{
		if (const FAnimSequenceEntry* Entry = AnimSequenceDataTable->FindRow<FAnimSequenceEntry>(RowName, TEXT("PickRandomAnimSequenceEntryForTag")))
		{
			if (Entry->Tag == Tag && Entry->Montage)
			{
				Candidates.Add(Entry);
			}
		}
	}

	if (Candidates.Num() == 0) return nullptr;
	return Candidates[FMath::RandHelper(Candidates.Num())];
}

void UMyProject1GameInstance::PlayAnimSequenceEvent(FName AnimEventID, ACharacter* PrimaryCharacter, AActor* SecondaryContextActor, bool bFadeInBeforeStart)
{
	if (!PrimaryCharacter || AnimEventID.IsNone() || !AnimEventDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceEvent: invalid arguments or AnimEventDataTable not set (AnimEventID=%s)"), *AnimEventID.ToString());
		OnAnimSequenceEventFinished.Broadcast(false);
		return;
	}

	FAnimEventDefinition* AnimEvent = AnimEventDataTable->FindRow<FAnimEventDefinition>(AnimEventID, TEXT("PlayAnimSequenceEvent"));
	if (!AnimEvent || AnimEvent->Steps.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayAnimSequenceEvent: AnimEventID '%s' not found in AnimEventDataTable, or has no Steps"), *AnimEventID.ToString());
		OnAnimSequenceEventFinished.Broadcast(false);
		return;
	}

	CurrentAnimEventID = AnimEventID;
	AnimEventPrimaryCharacter = PrimaryCharacter;
	AnimEventSecondaryContextActor = SecondaryContextActor;

	// FAnimSequenceEntry::MeshLocationOffset/MeshRotationOffset適用前の基準値をキャッシュしておく。
	// 各ステップはこの基準値+Offsetを都度設定するため、途中で値がズレていかない
	if (USkeletalMeshComponent* PrimaryMesh = PrimaryCharacter->GetMesh())
	{
		AnimEventPrimaryBaseMeshLocation = PrimaryMesh->GetRelativeLocation();
		AnimEventPrimaryBaseMeshRotation = PrimaryMesh->GetRelativeRotation();
	}
	if (ACharacter* SecondaryCharacter = Cast<ACharacter>(SecondaryContextActor))
	{
		if (USkeletalMeshComponent* SecondaryMesh = SecondaryCharacter->GetMesh())
		{
			AnimEventSecondaryBaseMeshLocation = SecondaryMesh->GetRelativeLocation();
			AnimEventSecondaryBaseMeshRotation = SecondaryMesh->GetRelativeRotation();
		}
	}

	// 再生中はマウスのカメラ操作以外（移動・アクション等）をロックする。
	// AMyProject1Character::DoLookはbIsInputLockedを見ないため、カメラ操作だけは引き続き可能
	bAnimEventOverrodeMusic = false;
	if (AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(PrimaryCharacter))
	{
		MyPrimaryCharacter->SetInputLocked(true);

		// EventBGMが設定されていればイベント中だけBGMをオーバーライドする（未設定ならフィールド/部屋BGMのまま何もしない）
		if (!AnimEvent->EventBGM.IsNull() && MyPrimaryCharacter->MusicComp)
		{
			bAnimEventOverrodeMusic = true;
			MyPrimaryCharacter->MusicComp->EnterRoomMusic(AnimEvent->EventBGM);
		}
	}

	if (bFadeInBeforeStart)
	{
		// ワープを経由しない直接呼び出し（会話等）用。ステップ切り替えと同じ暗転を開始前にも挟む
		TransitionToAnimEventStep(0);
	}
	else
	{
		PlayAnimEventStep(0);
	}
}

void UMyProject1GameInstance::PlayAnimEventStep(int32 StepIndex)
{
	GetTimerManager().ClearTimer(AnimEventStepDurationTimerHandle);
	GetTimerManager().ClearTimer(AnimEventStepFadeLeadTimerHandle);
	bAnimEventStepLooping = false;

	FAnimEventDefinition* AnimEvent = AnimEventDataTable
		? AnimEventDataTable->FindRow<FAnimEventDefinition>(CurrentAnimEventID, TEXT("PlayAnimEventStep"))
		: nullptr;

	if (!AnimEvent || !AnimEvent->Steps.IsValidIndex(StepIndex))
	{
		// 全ステップ再生完了：入力ロックを解除し、EventBGMでオーバーライドしていれば元のBGMに戻してから状態をクリアする
		if (AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(AnimEventPrimaryCharacter.Get()))
		{
			MyPrimaryCharacter->SetInputLocked(false);

			if (bAnimEventOverrodeMusic && MyPrimaryCharacter->MusicComp)
			{
				MyPrimaryCharacter->MusicComp->ExitRoomMusic();
			}
		}

		// MeshLocationOffset/MeshRotationOffsetで動かした分を、キャッシュしておいた基準値へ戻す
		if (ACharacter* PrimaryCharacter = AnimEventPrimaryCharacter.Get())
		{
			if (USkeletalMeshComponent* PrimaryMesh = PrimaryCharacter->GetMesh())
			{
				PrimaryMesh->SetRelativeLocation(AnimEventPrimaryBaseMeshLocation);
				PrimaryMesh->SetRelativeRotation(AnimEventPrimaryBaseMeshRotation);
			}
		}
		if (ACharacter* SecondaryCharacter = Cast<ACharacter>(AnimEventSecondaryContextActor.Get()))
		{
			if (USkeletalMeshComponent* SecondaryMesh = SecondaryCharacter->GetMesh())
			{
				SecondaryMesh->SetRelativeLocation(AnimEventSecondaryBaseMeshLocation);
				SecondaryMesh->SetRelativeRotation(AnimEventSecondaryBaseMeshRotation);
			}
		}

		bAnimEventOverrodeMusic = false;
		CurrentAnimEventStepIndex = INDEX_NONE;
		CurrentAnimEventID = NAME_None;
		AnimEventPrimaryCharacter.Reset();
		AnimEventSecondaryContextActor.Reset();
		CurrentAnimEventMontage.Reset();
		OnAnimSequenceEventFinished.Broadcast(true);
		return;
	}

	CurrentAnimEventStepIndex = StepIndex;
	const FAnimEventStep& Step = AnimEvent->Steps[StepIndex];

	ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(Step, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);

	// このステップの再生開始時に、同じTagを持つ候補（パンチ1/パンチ2等）から1回だけ抽選する。
	// bLoop中に終了→再生を繰り返す間は、HandleAnimEventStepMontageEndedがここで選ばれたMontageをそのまま再生し続ける（再抽選しない）
	const FAnimSequenceEntry* SelectedEntry = PickRandomAnimSequenceEntryForTag(AnimSequenceDataTable, Step.Tag);
	UAnimMontage* Montage = SelectedEntry ? SelectedEntry->Montage : nullptr;

	UAnimInstance* AnimInst = (TargetCharacter && TargetCharacter->GetMesh()) ? TargetCharacter->GetMesh()->GetAnimInstance() : nullptr;

	if (!Montage || !AnimInst)
	{
		// 再生対象・アセットのいずれかが見つからない場合は、このステップを飛ばして次へ進む
		PlayAnimEventStep(StepIndex + 1);
		return;
	}

	CurrentAnimEventMontage = Montage;

	// SelectedEntryのMeshLocationOffset/MeshRotationOffsetを、キャッシュしておいた基準値に加算して適用する。
	// PlayTargetがNPC（Secondary）かPlayer（Primary）かで参照する基準値を切り替える
	if (USkeletalMeshComponent* TargetMesh = TargetCharacter->GetMesh())
	{
		const bool bIsSecondaryTarget = (Step.PlayTarget == EStatTargetActor::NPC);
		const FVector& BaseLocation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshLocation : AnimEventPrimaryBaseMeshLocation;
		const FRotator& BaseRotation = bIsSecondaryTarget ? AnimEventSecondaryBaseMeshRotation : AnimEventPrimaryBaseMeshRotation;
		TargetMesh->SetRelativeLocation(BaseLocation + SelectedEntry->MeshLocationOffset);
		TargetMesh->SetRelativeRotation(BaseRotation + SelectedEntry->MeshRotationOffset);
	}

	PlayAnimEventStepMontage(AnimInst, Montage);

	// セリフが設定されていれば、再生開始と同時にログへ出す（bIsPlayerLine=trueならプレイヤー名付き、falseなら名前なし）
	if (!SelectedEntry->Line.IsEmpty())
	{
		FString LogMsg;
		if (SelectedEntry->bIsPlayerLine)
		{
			AMyProject1Character* MyPrimaryCharacter = Cast<AMyProject1Character>(AnimEventPrimaryCharacter.Get());
			FString PlayerName = (MyPrimaryCharacter && !MyPrimaryCharacter->MyStats.NPCName.IsEmpty())
				? MyPrimaryCharacter->MyStats.NPCName : TEXT("???");
			LogMsg = FString::Printf(TEXT("%s : %s"), *PlayerName, *SelectedEntry->Line.ToString());
		}
		else
		{
			LogMsg = SelectedEntry->Line.ToString();
		}

		if (IRpgCharacterInterface* RpgInterface = Cast<IRpgCharacterInterface>(TargetCharacter))
		{
			RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::Dialogue);
		}
	}

	if (Step.bLoop && Step.Duration > 0.0f)
	{
		bAnimEventStepLooping = true;

		// Durationいっぱいまでループさせてから暗転を始めると、暗転が完了しきる前にMontageを打ち切ることになり
		// Idleが透けて見えてしまう。暗転にかかる時間（WarpFadeOutDuration）を差し引いた時点で暗転を開始し、
		// その間もループを継続、暗転が完全に終わったタイミング（HandleAnimEventStepTransitionFadeComplete）で
		// 打ち切ることで、トータルの再生時間はDurationのまま、打ち切り自体は暗転で隠せる
		const float LoopFadeLeadTime = FMath::Max(Step.Duration - WarpFadeOutDuration, 0.0f);
		GetTimerManager().SetTimer(AnimEventStepDurationTimerHandle, this,
			&UMyProject1GameInstance::HandleAnimEventStepDurationTimeUp, LoopFadeLeadTime, false);
	}
	else
	{
		// 単発ステップ：Montage自体の残り時間がWarpFadeOutDuration秒を切るタイミングで暗転を開始する。
		// こうすることで、Montageが自然終了してIdleへ戻るのとほぼ同時に画面が完全に暗くなり、
		// 暗転が完了しきる前にIdleが透けて見える現象を防ぐ
		const float MontageLength = Montage->GetPlayLength();
		const float FadeLeadTime = FMath::Max(MontageLength - WarpFadeOutDuration, 0.0f);
		GetTimerManager().SetTimer(AnimEventStepFadeLeadTimerHandle, this,
			&UMyProject1GameInstance::HandleAnimEventStepFadeLeadTimeUp, FadeLeadTime, false);
	}
}

void UMyProject1GameInstance::HandleAnimEventStepDurationTimeUp()
{
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;

	// ここではまだモンタージュを止めない。次のステップへの暗転を開始するだけにして、
	// 実際の打ち切りは暗転が完全に終わった後（HandleAnimEventStepTransitionFadeComplete）まで遅らせる。
	// こうしないと、暗転が完了しきる前にIdleへ戻る瞬間が透けて見えてしまう
	TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
}

void UMyProject1GameInstance::HandleAnimEventStepMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;

	// Montage_Stopで打ち切った古いステップのモンタージュから遅延して届く終了通知は、
	// 既に次のステップへ進んだ後の状態（bAnimEventStepLooping・タイマー）と誤って結びついてしまうため、
	// 「今のステップで実際に再生しているモンタージュ」と一致しないものは無視する
	if (Montage != CurrentAnimEventMontage.Get()) return;

	if (bAnimEventStepLooping)
	{
		// ループの継ぎ目の再生し直しはHandleAnimEventStepMontageBlendingOut（ブレンドアウト開始時点）が
		// 既に行っている。ここで発火するEndは、その時点で再生し直した新しいインスタンスに対して
		// 役目を終えた「古いインスタンス」がブレンドアウトを完了した通知にすぎないため、何もしない
		return;
	}

	// 通常はHandleAnimEventStepMontageBlendingOut（ブレンドアウト開始時点）が既に次のステップへの
	// 暗転を始めている（PendingAnimEventNextStepIndexが立っている）。ここに来るのは、何らかの理由で
	// BlendingOut側が発火しなかった場合の保険
	if (PendingAnimEventNextStepIndex == INDEX_NONE)
	{
		TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
	}
}

void UMyProject1GameInstance::HandleAnimEventStepMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	// bInterrupted=trueはDuration経過（HandleAnimEventStepDurationTimeUp）等による明示的なMontage_Stop由来。
	// ループ・暗転のどちらも、その打ち切り処理側が既に次のステップへ進めるので、ここでは何もしない
	if (bInterrupted) return;
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;
	if (Montage != CurrentAnimEventMontage.Get()) return;

	if (bAnimEventStepLooping)
	{
		// bAnimEventStepLoopingがtrueの間は、Duration用タイマーが既に発火済み（＝暗転中）でも
		// 同じモンタージュを再生し直してループを継続する。実際に打ち切るのは暗転が完全に終わった
		// タイミング（HandleAnimEventStepTransitionFadeComplete、そこでbAnimEventStepLoopingをfalseにする）
		// なので、この関数がそれより後に呼ばれることはない
		FAnimEventDefinition* AnimEvent = AnimEventDataTable
			? AnimEventDataTable->FindRow<FAnimEventDefinition>(CurrentAnimEventID, TEXT("HandleAnimEventStepMontageBlendingOut"))
			: nullptr;

		if (!AnimEvent || !AnimEvent->Steps.IsValidIndex(CurrentAnimEventStepIndex)) return;

		const FAnimEventStep& Step = AnimEvent->Steps[CurrentAnimEventStepIndex];
		ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(Step, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);
		UAnimInstance* AnimInst = (TargetCharacter && TargetCharacter->GetMesh()) ? TargetCharacter->GetMesh()->GetAnimInstance() : nullptr;

		if (AnimInst && Montage)
		{
			// ブレンドアウトが完了しきる前（まだウェイトが高いうち）に次の再生を仕込むことで、
			// 完全にIdleへ戻ってから再生し直す場合よりもループの継ぎ目を目立たなくする
			// （新旧2つのMontageインスタンスが同時に存在し、クロスフェードする形になる）
			PlayAnimEventStepMontage(AnimInst, Montage);
		}
		return;
	}

	// ループなしステップ：ブレンドアウトが完了してIdleへ戻りきる前（HandleAnimEventStepMontageEndedの
	// 発火より早いタイミング）で暗転を始めることで、暗転が始まる前にIdleへ一瞬戻って見える現象を防ぐ
	if (PendingAnimEventNextStepIndex == INDEX_NONE)
	{
		TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
	}
}

void UMyProject1GameInstance::PlayAnimEventStepMontage(UAnimInstance* AnimInst, UAnimMontage* Montage)
{
	AnimInst->Montage_Play(Montage);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUFunction(this, FName(TEXT("HandleAnimEventStepMontageEnded")));
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUFunction(this, FName(TEXT("HandleAnimEventStepMontageBlendingOut")));
	AnimInst->Montage_SetBlendingOutDelegate(BlendingOutDelegate, Montage);
}

void UMyProject1GameInstance::HandleAnimEventStepFadeLeadTimeUp()
{
	if (CurrentAnimEventStepIndex == INDEX_NONE) return;
	if (bAnimEventStepLooping) return; // ループ中はDurationタイマー側が進行を管理するため対象外
	if (PendingAnimEventNextStepIndex != INDEX_NONE) return; // 既に他経路で暗転済みなら何もしない

	TransitionToAnimEventStep(CurrentAnimEventStepIndex + 1);
}

void UMyProject1GameInstance::TransitionToAnimEventStep(int32 NextStepIndex)
{
	PendingAnimEventNextStepIndex = NextStepIndex;

	// ワープと同じ暗転合図をUIへ送る（試験実装）。WBP_LoadingScreen側が暗転→維持→明転を自走してくれる想定
	OnWarpFadeOutRequested.Broadcast();

	GetTimerManager().SetTimer(AnimEventStepTransitionTimerHandle, this,
		&UMyProject1GameInstance::HandleAnimEventStepTransitionFadeComplete, WarpFadeOutDuration, false);
}

void UMyProject1GameInstance::HandleAnimEventStepTransitionFadeComplete()
{
	const int32 NextStepIndex = PendingAnimEventNextStepIndex;
	PendingAnimEventNextStepIndex = INDEX_NONE;

	// ループ中のステップから抜ける場合は、暗転が完全に終わった今のタイミングでモンタージュを打ち切る。
	// Montageを明示して打ち切ることで、直後にPlayAnimEventStepが再生する次のモンタージュを巻き込まない
	if (bAnimEventStepLooping)
	{
		FAnimEventDefinition* AnimEvent = AnimEventDataTable
			? AnimEventDataTable->FindRow<FAnimEventDefinition>(CurrentAnimEventID, TEXT("HandleAnimEventStepTransitionFadeComplete"))
			: nullptr;

		if (AnimEvent && AnimEvent->Steps.IsValidIndex(CurrentAnimEventStepIndex))
		{
			const FAnimEventStep& Step = AnimEvent->Steps[CurrentAnimEventStepIndex];
			ACharacter* TargetCharacter = ResolveAnimEventTargetCharacter(Step, AnimEventPrimaryCharacter, AnimEventSecondaryContextActor);
			if (UAnimInstance* AnimInst = (TargetCharacter && TargetCharacter->GetMesh()) ? TargetCharacter->GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInst->Montage_Stop(0.1f, CurrentAnimEventMontage.Get());
			}
		}

		bAnimEventStepLooping = false;
	}

	PlayAnimEventStep(NextStepIndex);
}

// ----------------------------------------------------
// イベント分岐システム側の統合：ClearCondition=AnimationSequence
// ----------------------------------------------------
void UMyProject1GameInstance::BeginAnimEventSequenceIfNeeded()
{
	if (!bHasActiveEvent || bAnimEventSequenceStarted || !EventDefinitionDataTable) return;

	FEventDefinition* Definition = EventDefinitionDataTable->FindRow<FEventDefinition>(ActiveEventID, TEXT("BeginAnimEventSequenceIfNeeded"));
	if (!Definition || Definition->ClearCondition != EEventClearCondition::AnimationSequence) return;

	bAnimEventSequenceStarted = true;

	if (Definition->AnimEventID.IsNone())
	{
		// アニメーションイベントが未設定なら、演出なしでそのまま成立させる（安全策）
		ResolveActiveEvent(true);
		return;
	}

	if (!OnAnimSequenceEventFinished.IsAlreadyBound(this, &UMyProject1GameInstance::HandleAnimSequenceEventFinishedForActiveEvent))
	{
		OnAnimSequenceEventFinished.AddDynamic(this, &UMyProject1GameInstance::HandleAnimSequenceEventFinishedForActiveEvent);
	}

	PlayAnimSequenceEvent(Definition->AnimEventID, ActiveEventPlayer.Get(), ActiveEventContextActor.Get());
}

void UMyProject1GameInstance::HandleAnimSequenceEventFinishedForActiveEvent(bool bCompletedNormally)
{
	// bAnimEventSequenceStartedは「進行中イベント経由でPlayAnimSequenceEventを開始した」場合だけtrueになる。
	// 単体でのPlayAnimSequenceEvent呼び出し（会話・QuestItemPoint等、イベントを経由しない再生）の完了時は
	// bHasActiveEventがfalse、もしくはこのフラグがfalseのままなので何もしない
	if (!bHasActiveEvent || !bAnimEventSequenceStarted) return;

	ResolveActiveEvent(true);
}

