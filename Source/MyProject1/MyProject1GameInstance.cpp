#include "MyProject1GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "MyProject1Character.h"
#include "MyProject1SaveGame.h"
#include "InventoryComponent.h"
#include "QuestComponent.h"
#include "SkinOverlayComponent.h"
#include "GameFramework/PlayerController.h"


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
	if (!PlayerCharacter || !WarpDataTable || WarpID.IsNone()) return;

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
// 1.5. ワープを伴わない「暗転を挟んだフラグ付与」の要求（NPCの表示切替など向け）
// ----------------------------------------------------
void UMyProject1GameInstance::RequestFadeThenGrantFlag(FName FlagName, AMyProject1Character* TargetCharacter)
{
	if (FlagName.IsNone() || !TargetCharacter) return;

	ReservedFlagToGrant = FlagName;
	ReservedFlagGrantTarget = TargetCharacter;

	// エリアChangeと全く同じ合図を使う。WBP_LoadingScreen側の変更は不要
	OnWarpFadeOutRequested.Broadcast();
}

// ----------------------------------------------------
// 2. 暗転が終わった後に呼ばれる「実際の移動」
// ----------------------------------------------------
void UMyProject1GameInstance::ExecuteWarpProcess()
{
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

	return UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, 0);
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

void UMyProject1GameInstance::ApplyPendingCharacterLoad(AMyProject1Character* Character)
{
	if (!Character || !PendingLoadSaveGame) return;

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
}

