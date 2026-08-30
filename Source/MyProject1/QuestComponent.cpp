#include "QuestComponent.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject1GameInstance.h"
#include "MyProject1HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"

#pragma execution_character_set("utf-8")

UQuestComponent::UQuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 毎フレーム処理は不要なので軽くする
}

void UQuestComponent::BeginPlay()
{
	Super::BeginPlay();

	// 日付が変わるたびに制限時間切れのクエストが無いか確認する。
	// AdvanceTimeBy（睡眠・待機）も日跨ぎ分だけAdvanceDayを呼ぶので、まとめて何日も進めても取りこぼさない
	if (UWorld* World = GetWorld())
	{
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(World->GetGameInstance()))
		{
			if (!GameInst->OnDayChangedDelegate.IsAlreadyBound(this, &UQuestComponent::CheckQuestTimeLimits))
			{
				GameInst->OnDayChangedDelegate.AddDynamic(this, &UQuestComponent::CheckQuestTimeLimits);
			}
		}
	}
}

void UQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(World->GetGameInstance()))
		{
			GameInst->OnDayChangedDelegate.RemoveDynamic(this, &UQuestComponent::CheckQuestTimeLimits);
		}
	}

	Super::EndPlay(EndPlayReason);
}

AMyProject1HUD* UQuestComponent::GetOwnerHUD() const
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			return Cast<AMyProject1HUD>(PC->GetHUD());
		}
	}
	return nullptr;
}

USoundBase* UQuestComponent::ResolveQuestSound(USoundBase* RowSound, USoundBase* AMyProject1HUD::* DefaultField) const
{
	// DT_QuestData側に個別設定があればそちらを優先し、無ければHUDの既定音にフォールバックする
	if (RowSound)
	{
		return RowSound;
	}
	if (AMyProject1HUD* HUD = GetOwnerHUD())
	{
		return HUD->*DefaultField;
	}
	return nullptr;
}

void UQuestComponent::ClearObjectiveClearedFlag(const FQuestData& Data) const
{
	if (Data.ObjectiveClearedFlag.IsNone()) return;

	// ObjectiveClearedFlagは「目的達成〜報告／放棄／再受注」の間だけ立てる一時トリガーとして扱う。
	// 恒久的に残したい称号はRewardFlagを使うこと
	if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner()))
	{
		OwnerChar->RemoveFlag(Data.ObjectiveClearedFlag);
	}
}

bool UQuestComponent::GetQuestData(FName QuestID, FQuestData& OutData)
{
	if (QuestID.IsNone() || !QuestDataTable) return false;

	FQuestData* Data = QuestDataTable->FindRow<FQuestData>(QuestID, TEXT("QuestContext"));
	if (Data)
	{
		OutData = *Data;
		return true;
	}
	return false;
}

EQuestStatus UQuestComponent::GetQuestStatus(FName QuestID)
{
	// 1. 進行中リストにあるかチェック（最優先）
	for (const FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.QuestID == QuestID)
		{
			return Progress.Status;
		}
	}

	// 2. すでにクリア済みかチェック ＆ クールタイム判定
	for (int32 i = 0; i < CompletedQuests.Num(); ++i)
	{
		if (CompletedQuests[i].QuestID == QuestID)
		{
			FQuestData Data;
			if (GetQuestData(QuestID, Data) && Data.bIsRepeatable)
			{
				// リピート可能なら日数を計算
				UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance());
				if (GameInst)
				{
					// 現在の総経過日数 - クリアした時の総経過日数
					int32 DaysPassed = GameInst->TotalElapsedDays - CompletedQuests[i].CompletedTotalDays;

					if (DaysPassed >= Data.CooldownDays)
					{
						// ★クールタイム明け！履歴から削除して「未受注」として返す
						CompletedQuests.RemoveAt(i);
						return EQuestStatus::NotStarted;
					}
				}
			}
			return EQuestStatus::Completed;
		}
	}

	// 3. どちらにも無ければ未受注
	return EQuestStatus::NotStarted;
}

bool UQuestComponent::CanAcceptQuest(FName QuestID)
{
	FString UnusedReason;
	return CanAcceptQuest(QuestID, UnusedReason);
}

bool UQuestComponent::CanAcceptQuest(FName QuestID, FString& OutFailReason)
{
	// すでに受注している、またはクリア済みなら受けられない
	const EQuestStatus Status = GetQuestStatus(QuestID);
	if (Status != EQuestStatus::NotStarted)
	{
		if (Status == EQuestStatus::Completed)
		{
			// リピート可能クエストなら「クールダウン中（残り日数）」、そうでなければ「クリア済みで再受注不可」
			FQuestData CompletedData;
			bool bReasonSet = false;
			if (GetQuestData(QuestID, CompletedData) && CompletedData.bIsRepeatable)
			{
				for (const FCompletedQuestInfo& Completed : CompletedQuests)
				{
					if (Completed.QuestID == QuestID)
					{
						if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
						{
							const int32 DaysPassed = GameInst->TotalElapsedDays - Completed.CompletedTotalDays;
							const int32 RemainingDays = FMath::Max(CompletedData.CooldownDays - DaysPassed, 0);
							OutFailReason = FString::Printf(TEXT("受注クールダウン中です（あと%d日）。"), RemainingDays);
							bReasonSet = true;
						}
						break;
					}
				}
			}
			if (!bReasonSet)
			{
				OutFailReason = TEXT("このクエストはすでにクリア済みです。");
			}
		}
		else
		{
			OutFailReason = TEXT("このクエストはすでに受注中です。");
		}
		return false;
	}

	// QuestIDが空欄の場合は「クエスト管理をしない、会話専用のエントリ」とみなし、常に表示可能とする
	// （GetQuestStatusは常にNotStartedを返し続けるので、NPCは毎回同じ固定セリフを表示できる）
	if (QuestID.IsNone())
	{
		return true;
	}

	FQuestData Data;
	if (!GetQuestData(QuestID, Data))
	{
		OutFailReason = TEXT("クエストデータが見つかりません。");
		return false;
	}

	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
	if (!OwnerChar)
	{
		OutFailReason = TEXT("受注条件を確認できませんでした。");
		return false;
	}

	// 受注条件1：フラグ（称号）
	if (!Data.RequiredFlag.IsNone() && !OwnerChar->HasFlag(Data.RequiredFlag))
	{
		OutFailReason = TEXT("受注条件が未達です（必要な称号を持っていません）。");
		return false;
	}

	// 受注条件2：前提クエスト（種別問わず。全て一度はクリア済みである必要がある）
	for (const FName& ReqQuestID : Data.PrerequisiteQuestIDs)
	{
		if (!EverCompletedQuestIDs.Contains(ReqQuestID))
		{
			OutFailReason = TEXT("受注条件が未達です（前提クエストが未クリアです）。");
			return false;
		}
	}

	// 受注条件3：数値ステータス条件（レベル・Karma等、全てAND判定）
	if (Data.RequiredStats.Num() > 0)
	{
		const FCharacterStats& Stats = OwnerChar->GetCharacterStats();
		for (const FQuestStatRequirement& Req : Data.RequiredStats)
		{
			float StatValue = 0.0f;
			if (!Stats.TryGetStatValue(Req.StatName, StatValue))
			{
				// 存在しないステータス名が指定された場合は設定ミスとみなし、安全側（受注不可）に倒す
				UE_LOG(LogTemp, Warning, TEXT("【Quest Error】クエスト「%s」のRequiredStatsに未知のステータス名 '%s' が指定されています。"), *QuestID.ToString(), *Req.StatName.ToString());
				OutFailReason = TEXT("受注条件が未達です（ステータス条件の設定を確認してください）。");
				return false;
			}

			bool bPass = true;
			switch (Req.CompareOp)
			{
			case EStatCompareOp::GreaterOrEqual: bPass = (StatValue >= Req.RequiredValue); break;
			case EStatCompareOp::LessOrEqual:    bPass = (StatValue <= Req.RequiredValue); break;
			case EStatCompareOp::Equal:          bPass = FMath::IsNearlyEqual(StatValue, Req.RequiredValue); break;
			case EStatCompareOp::Greater:        bPass = (StatValue > Req.RequiredValue); break;
			case EStatCompareOp::Less:           bPass = (StatValue < Req.RequiredValue); break;
			}

			if (!bPass)
			{
				OutFailReason = FString::Printf(TEXT("受注条件が未達です（%s が条件を満たしていません）。"), *Req.StatName.ToString());
				return false;
			}
		}
	}

	return true;
}

FName UQuestComponent::GetNextOfferableQuest(const TArray<FName>& CandidateQuestIDs)
{
	// 1. 進行中/報告待ちの候補があれば会話の続きとして最優先で返す
	for (const FName& ID : CandidateQuestIDs)
	{
		const EQuestStatus Status = GetQuestStatus(ID);
		if (Status == EQuestStatus::InProgress || Status == EQuestStatus::ObjectiveCleared)
		{
			return ID;
		}
	}

	// 2. 未受注（NotStarted）の候補があれば、優先順位（配列の先頭）通りに最初の1件を返す。
	// 受注条件を満たしているかどうかはここでは見ない（Locked/NotStarted行のどちらを出すかは呼び出し側の役割）。
	// こうしないと、連鎖の先頭クエストが条件未達の間、条件判定でスキップされて末尾のCompletedフレーバーに
	// 誤って飛んでしまう
	for (const FName& ID : CandidateQuestIDs)
	{
		if (GetQuestStatus(ID) == EQuestStatus::NotStarted)
		{
			return ID;
		}
	}

	// 3. 何も提示できない場合（候補が全てCompleted等）は、リスト末尾の状態をフォールバックとして返す
	if (CandidateQuestIDs.Num() > 0)
	{
		return CandidateQuestIDs.Last();
	}

	return NAME_None;
}

bool UQuestComponent::AcceptQuest(FName QuestID)
{
	// 受注条件を満たしていなければ受けられない
	FString FailReason;
	if (!CanAcceptQuest(QuestID, FailReason))
	{
		if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner()))
		{
			// 条件を満たしていない場合はシステムログを出して中断（理由が取れなければ汎用メッセージにフォールバック）
			OwnerChar->OnReceiveLogMessage(FailReason.IsEmpty() ? TEXT("受注条件が未達です。") : FailReason, ELogMessageType::System);
		}
		return false;
	}

	FQuestData Data;
	if (!GetQuestData(QuestID, Data))
	{
		return false;
	}

	// 進行中リストに追加
	FQuestProgress NewQuest(QuestID);

	// 制限時間の判定起点として、受注時点の累計日数を記録する（TimeLimitDaysが未設定でも害はない）
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
	{
		NewQuest.AcceptedTotalDays = GameInst->TotalElapsedDays;
	}

	ActiveQuests.Add(NewQuest);

	CheckInitialGatherProgress(QuestID);
	CheckInitialAchievementProgress(QuestID);

	// UIに通知
	OnQuestUpdated.Broadcast(QuestID);

	// ログ表示
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
	if (OwnerChar)
	{
		FString LogMsg = FString::Printf(TEXT("クエスト「%s」を受注した。"), *Data.QuestName.ToString());
		OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);

		// 受注時フラグの付与（例：AQuestItemPointのRequiredFlagをこれに合わせておくと、受注と同時に表示される）
		if (!Data.AcceptFlag.IsNone())
		{
			OwnerChar->AddFlag(Data.AcceptFlag);
		}
	}

	// 前周の名残でObjectiveClearedFlagが残っていても、受注した時点で必ずクリーンな状態から始める
	// （周回クエストで対象NPCSpawnerを再武装させるため。手動でのフラグ削除設定を不要にする）
	ClearObjectiveClearedFlag(Data);

	if (USoundBase* Sound = ResolveQuestSound(Data.AcceptSound, &AMyProject1HUD::DefaultQuestAcceptSound))
	{
		UGameplayStatics::PlaySound2D(GetWorld(), Sound);
	}

	return true;
}

bool UQuestComponent::CancelQuest(FName QuestID)
{
	for (int32 i = 0; i < ActiveQuests.Num(); ++i)
	{
		if (ActiveQuests[i].QuestID != QuestID) continue;

		// 既に条件達成済み（報告待ち）でも、報告前ならまだ放棄できる仕様
		ActiveQuests.RemoveAt(i);

		// UIに通知
		OnQuestUpdated.Broadcast(QuestID);

		// ログ表示
		FQuestData Data;
		if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner()))
		{
			if (GetQuestData(QuestID, Data))
			{
				FString LogMsg = FString::Printf(TEXT("クエスト「%s」を放棄した。"), *Data.QuestName.ToString());
				OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);

				// 放棄した場合も対象出現トリガー(ObjectiveClearedFlag)は残さない（周回・別受注の妨げになるため）
				ClearObjectiveClearedFlag(Data);
			}
		}

		return true;
	}

	// 進行中でないクエストは放棄しようがない
	return false;
}

void UQuestComponent::UpdateKillObjective(FName EnemyID)
{
	bool bUpdatedAny = false;

	// 進行中の全クエストをチェック
	for (FQuestProgress& Progress : ActiveQuests)
	{
		// 進行中でない場合はスキップ
		if (Progress.Status != EQuestStatus::InProgress) continue;

		FQuestData Data;
		if (GetQuestData(Progress.QuestID, Data))
		{
			// 討伐クエストであり、かつ倒した敵のIDが目標と一致するか？
			if (Data.QuestType == EQuestType::Kill && Data.TargetID == EnemyID)
			{
				// カウントを増やす
				Progress.CurrentAmount++;
				bUpdatedAny = true;

				AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

				// 目標数に達したか？
				if (Progress.CurrentAmount >= Data.RequiredAmount)
				{
					Progress.CurrentAmount = Data.RequiredAmount; // 上限で止める
					Progress.Status = EQuestStatus::ObjectiveCleared; // 報告待ち状態へ

					// 目的達成フラグの付与（報告を待たずにNPCSpawner等を反応させる用）
					if (OwnerChar && !Data.ObjectiveClearedFlag.IsNone())
					{
						OwnerChar->AddFlag(Data.ObjectiveClearedFlag);
					}

					if (OwnerChar)
					{
						FString LogMsg = FString::Printf(TEXT("クエスト「%s」の目的を達成した！"), *Data.QuestName.ToString());
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
					}

					if (USoundBase* Sound = ResolveQuestSound(Data.ObjectiveClearedSound, &AMyProject1HUD::DefaultQuestObjectiveClearedSound))
					{
						UGameplayStatics::PlaySound2D(GetWorld(), Sound);
					}
				}
				else
				{
					// 途中経過のログ
					if (OwnerChar)
					{
						FString LogMsg = FString::Printf(TEXT("%sを倒した（%d / %d）"), *EnemyID.ToString(), Progress.CurrentAmount, Data.RequiredAmount);
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);
					}
				}
			}

			// Deliveryクエストの討伐パート（DeliveryKillTargetID設定時のみ。会話パート完了後だけカウントする）
			if (Data.QuestType == EQuestType::Delivery
				&& !Data.DeliveryKillTargetID.IsNone()
				&& Data.DeliveryKillTargetID == EnemyID)
			{
				// まだ情報収集中なら討伐は数えない（先に対象を倒しても進めさせない）
				if (Progress.CurrentAmount < Data.RequiredTalkNPCIDs.Num())
				{
					continue;
				}

				const int32 NeedKills = FMath::Max(1, Data.DeliveryKillAmount);
				Progress.DeliveryKillCount++;
				bUpdatedAny = true;

				AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

				if (Progress.DeliveryKillCount >= NeedKills)
				{
					Progress.DeliveryKillCount = NeedKills; // 上限で止める
					Progress.Status = EQuestStatus::ObjectiveCleared; // ここで初めて報告待ちへ

					// 対象を討伐した時点で出現トリガーは役目終了。ここで落とすことで、
					// 報告前に別レベルへ出入りしても対象が重複POPしなくなる
					ClearObjectiveClearedFlag(Data);

					if (OwnerChar)
					{
						FString LogMsg = FString::Printf(TEXT("クエスト「%s」の目的を達成した！"), *Data.QuestName.ToString());
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
					}

					if (USoundBase* Sound = ResolveQuestSound(Data.ObjectiveClearedSound, &AMyProject1HUD::DefaultQuestObjectiveClearedSound))
					{
						UGameplayStatics::PlaySound2D(GetWorld(), Sound);
					}
				}
				else if (OwnerChar)
				{
					FString LogMsg = FString::Printf(TEXT("%sを討伐した（%d / %d）"), *EnemyID.ToString(), Progress.DeliveryKillCount, NeedKills);
					OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);
				}
			}
		}
	}

	// 1つでも進行したクエストがあればUIに通知
	if (bUpdatedAny)
	{
		OnQuestUpdated.Broadcast(NAME_None); // 全体更新
	}
}

bool UQuestComponent::ReportQuest(FName QuestID)
{
	// 進行中リストから該当クエストを探す
	for (int32 i = 0; i < ActiveQuests.Num(); ++i)
	{
		if (ActiveQuests[i].QuestID == QuestID)
		{
			// 「条件達成」状態でないと報告できない
			if (ActiveQuests[i].Status != EQuestStatus::ObjectiveCleared)
			{
				return false;
			}

			FQuestData Data;
			if (GetQuestData(QuestID, Data))
			{
				AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
				if (OwnerChar)
				{
					// ★ ここで InvComp を先に取得しておく！
					UInventoryComponent* InvComp = OwnerChar->FindComponentByClass<UInventoryComponent>();
					
					if (Data.QuestType == EQuestType::Gather && InvComp)
					{
						// RemoveItem が失敗した（間違って捨ててしまって数が足りない等）場合は報告中断
						if (!InvComp->RemoveItem(Data.TargetID, Data.RequiredAmount))
						{
							if (OwnerChar) OwnerChar->OnReceiveLogMessage(TEXT("アイテムが足りません。"), ELogMessageType::System);
							return false;
						}
					}

					// --- 報酬の付与 ---
					// 1. 経験値
					if (Data.RewardExperience > 0)
					{
						OwnerChar->AddExperience(Data.RewardExperience);
					}

					// 2. ギル（インベントリを使用）
					if (InvComp)
					{
						if (Data.RewardGil > 0)
						{
							InvComp->AddGil(Data.RewardGil);
							FString GilMsg = FString::Printf(TEXT("報酬として %d￥ 手に入れた。"), Data.RewardGil);
							OwnerChar->OnReceiveLogMessage(GilMsg, ELogMessageType::System);
						}

						// 3. アイテム
						if (!Data.RewardItemID.IsNone() && Data.RewardItemAmount > 0)
						{
							// アイテムをカバンに追加
							InvComp->AddItem(Data.RewardItemID, Data.RewardItemAmount);

							// インベントリからアイテムのデータを取得して名前を取り出す
							FItemData ItemInfo;
							if (InvComp->GetItemDataBP(Data.RewardItemID, ItemInfo))
							{
								// ログの作成（個数が1個でも複数でも対応できる形にしています）
								FString ItemMsg = FString::Printf(TEXT("報酬として %s を%d個手に入れた。"), *ItemInfo.Name, Data.RewardItemAmount);

								// ギルと同じくシステムメッセージとして送信
								OwnerChar->OnReceiveLogMessage(ItemMsg, ELogMessageType::System);
							}

							// 4. 称号（フラグ）の付与
							if (!Data.RewardFlag.IsNone())
							{
								OwnerChar->AddFlag(Data.RewardFlag);

								// 称号獲得のログ表示
								FString TitleMsg = FString::Printf(TEXT("称号「%s」を獲得した！"), *Data.RewardFlag.ToString());
								OwnerChar->OnReceiveLogMessage(TitleMsg, ELogMessageType::System);
							}

						}
					}

					// 完了ログ
					FString LogMsg = FString::Printf(TEXT("クエスト「%s」をコンプリートした！"), *Data.QuestName.ToString());
					OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);

					if (USoundBase* Sound = ResolveQuestSound(Data.CompletionSound, &AMyProject1HUD::DefaultQuestCompletionSound))
					{
						// 2Dサウンド（距離に関係なく画面全体に聞こえる音）として再生
						UGameplayStatics::PlaySound2D(GetWorld(), Sound);
					}
				}

				// 対象出現トリガー(ObjectiveClearedFlag)は報告完了で役目を終えるので必ず落とす。
				// これで周回時のNPCSpawner再武装がダイアログ/CompletionRemoveFlagの手動設定なしで成立する
				ClearObjectiveClearedFlag(Data);

				// 周回用：報告完了時に、上記とは別の任意フラグも削除する（世界状態のリセット等の汎用用途）
				if (OwnerChar && !Data.CompletionRemoveFlag.IsNone())
				{
					OwnerChar->RemoveFlag(Data.CompletionRemoveFlag);
				}

				// リストの移動（進行中から消して、完了履歴に追加）
				ActiveQuests.RemoveAt(i);

				FCompletedQuestInfo NewRecord;
				NewRecord.QuestID = QuestID;
				if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
				{
					NewRecord.CompletedTotalDays = GameInst->TotalElapsedDays;
				}
				CompletedQuests.Add(NewRecord);

				// クールタイムでCompletedQuestsから消えても実績判定が壊れないよう、恒久的な履歴にも記録する
				EverCompletedQuestIDs.AddUnique(QuestID);
				UpdateAchievementObjective(QuestID);

				OnQuestUpdated.Broadcast(QuestID);
				return true;
			}
		}
	}

	return false;
}

bool UQuestComponent::Debug_ForceCompleteQuest(FName QuestID)
{
	// Ctrlキーが押されている時だけ有効（通常クリックでは何もしない安全策）
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PC = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PC || !(PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl)))
	{
		return false;
	}

	for (FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.QuestID == QuestID)
		{
			// 条件未達成でもデバッグで強制的に「条件達成」扱いにしてから、通常の報告処理に流す
			Progress.Status = EQuestStatus::ObjectiveCleared;
			return ReportQuest(QuestID);
		}
	}
	return false;
}

void UQuestComponent::UpdateGatherObjective(FName ItemID, int32 AmountAdded)
{
	bool bUpdatedAny = false;
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

	for (FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.Status != EQuestStatus::InProgress) continue;

		FQuestData Data;
		if (GetQuestData(Progress.QuestID, Data))
		{
			// 収集クエストであり、対象アイテムが一致するか
			if (Data.QuestType == EQuestType::Gather && Data.TargetID == ItemID)
			{
				Progress.CurrentAmount += AmountAdded;
				bUpdatedAny = true;

				if (Progress.CurrentAmount >= Data.RequiredAmount)
				{
					Progress.CurrentAmount = Data.RequiredAmount; // 上限で止める
					Progress.Status = EQuestStatus::ObjectiveCleared; // 報告待ち状態へ

					// 目的達成フラグの付与（報告を待たずにNPCSpawner等を反応させる用）
					if (OwnerChar && !Data.ObjectiveClearedFlag.IsNone())
					{
						OwnerChar->AddFlag(Data.ObjectiveClearedFlag);
					}

					if (OwnerChar) {
						FString LogMsg = FString::Printf(TEXT("クエスト「%s」のアイテムが集まった！"), *Data.QuestName.ToString());
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
					}

					if (USoundBase* Sound = ResolveQuestSound(Data.ObjectiveClearedSound, &AMyProject1HUD::DefaultQuestObjectiveClearedSound))
					{
						UGameplayStatics::PlaySound2D(GetWorld(), Sound);
					}
				}
				else
				{
					// 途中経過のログ
					if (OwnerChar) {
						FString LogMsg = FString::Printf(TEXT("%sを手に入れた（%d / %d）"), *ItemID.ToString(), Progress.CurrentAmount, Data.RequiredAmount);
						OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);
					}
				}
			}
		}
	}
	if (bUpdatedAny) OnQuestUpdated.Broadcast(NAME_None);
}

void UQuestComponent::UpdateTalkObjective(FName QuestID, AActor* TalkedToNPC)
{
	if (QuestID.IsNone() || !TalkedToNPC) return;

	for (FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.QuestID != QuestID || Progress.Status != EQuestStatus::InProgress) continue;

		FQuestData Data;
		if (!GetQuestData(QuestID, Data) || Data.QuestType != EQuestType::Delivery) break;

		const int32 RequiredCount = Data.RequiredTalkNPCIDs.Num();
		AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

		if (Data.bUnorderedTalk)
		{
			// 順不同：RequiredTalkNPCIDsのうち、まだ集めていないTagを持っているかを探す
			FName MatchedTag = NAME_None;
			for (const FName& ReqTag : Data.RequiredTalkNPCIDs)
			{
				if (TalkedToNPC->ActorHasTag(ReqTag) && !Progress.CollectedTalkIDs.Contains(ReqTag))
				{
					MatchedTag = ReqTag;
					break;
				}
			}

			// 対象外（無関係なNPC/ポイント）か、既にインタラクト済みのポイントを再度触っただけなら何もしない
			if (MatchedTag.IsNone())
			{
				break;
			}

			Progress.CollectedTalkIDs.Add(MatchedTag);
			Progress.CurrentAmount = Progress.CollectedTalkIDs.Num();
		}
		else
		{
			// 次に期待している相手のTagを持っていないなら何もしない（順番を飛ばして進めさせない）
			if (!Data.RequiredTalkNPCIDs.IsValidIndex(Progress.CurrentAmount) || !TalkedToNPC->ActorHasTag(Data.RequiredTalkNPCIDs[Progress.CurrentAmount]))
			{
				break;
			}

			Progress.CurrentAmount++;
		}

		if (Progress.CurrentAmount >= RequiredCount)
		{
			// 目的達成フラグの付与（報告を待たずにNPCSpawner等を反応させる用。討伐パートの有無に関わらず会話完了時点で出す）
			if (OwnerChar && !Data.ObjectiveClearedFlag.IsNone())
			{
				OwnerChar->AddFlag(Data.ObjectiveClearedFlag);
			}

			const bool bHasKillPart = !Data.DeliveryKillTargetID.IsNone();

			if (bHasKillPart)
			{
				// 会話（情報収集）パートは完了。討伐パートが残っているのでStatusはまだ報告待ちにしない
				if (OwnerChar)
				{
					const FString Msg = Data.DeliveryTalkCompleteLog.IsEmpty()
						? FString::Printf(TEXT("クエスト「%s」：情報を集め終えた。"), *Data.QuestName.ToString())
						: Data.DeliveryTalkCompleteLog.ToString();
					OwnerChar->OnReceiveLogMessage(Msg, ELogMessageType::System);
				}
			}
			else
			{
				Progress.Status = EQuestStatus::ObjectiveCleared; // 依頼主への報告待ち状態へ

				if (OwnerChar)
				{
					FString LogMsg = FString::Printf(TEXT("クエスト「%s」の目的を達成した！"), *Data.QuestName.ToString());
					OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
				}

				if (USoundBase* Sound = ResolveQuestSound(Data.ObjectiveClearedSound, &AMyProject1HUD::DefaultQuestObjectiveClearedSound))
				{
					UGameplayStatics::PlaySound2D(GetWorld(), Sound);
				}
			}
		}
		else if (OwnerChar)
		{
			FString LogMsg = FString::Printf(TEXT("クエスト「%s」の進捗（%d / %d）"), *Data.QuestName.ToString(), Progress.CurrentAmount, RequiredCount);
			OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);
		}

		OnQuestUpdated.Broadcast(QuestID);
		break;
	}
}

bool UQuestComponent::IsExpectedTalkTarget(FName QuestID, AActor* NPC)
{
	if (QuestID.IsNone() || !NPC) return true;

	for (const FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.QuestID != QuestID || Progress.Status != EQuestStatus::InProgress) continue;

		FQuestData Data;
		if (!GetQuestData(QuestID, Data) || Data.QuestType != EQuestType::Delivery) return true;

		// このNPCがそもそもRequiredTalkNPCIDsのいずれのTagも持っていないなら、
		// 依頼主など「会話チェーンの対象外」のNPCとみなし、順番チェックをスキップする
		// （依頼主にはTagを付けない設計のため、チェックしないと受注後に再度話しかけた時
		// 「順番違い」と誤判定されてNotStarted行＝再受注の勧誘が出てしまう）
		bool bIsChainMember = false;
		for (const FName& Tag : Data.RequiredTalkNPCIDs)
		{
			if (NPC->ActorHasTag(Tag))
			{
				bIsChainMember = true;
				break;
			}
		}
		if (!bIsChainMember) return true;

		// 順不同モードでは「次はこの相手」という概念がないので、チェーンの一員なら常にOK扱いにする
		if (Data.bUnorderedTalk) return true;

		return Data.RequiredTalkNPCIDs.IsValidIndex(Progress.CurrentAmount) && NPC->ActorHasTag(Data.RequiredTalkNPCIDs[Progress.CurrentAmount]);
	}

	return true;
}

void UQuestComponent::UpdateAchievementObjective(FName CompletedQuestID)
{
	bool bUpdatedAny = false;
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

	for (FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.Status != EQuestStatus::InProgress) continue;

		FQuestData Data;
		if (!GetQuestData(Progress.QuestID, Data)) continue;

		// 実績クエストであり、今クリアされたクエストが前提リストに含まれているか？
		if (Data.QuestType != EQuestType::Achievement || !Data.RequiredQuestIDs.Contains(CompletedQuestID)) continue;

		// 差分加算ではなく、前提クエストのうち「一度でもクリアした数」を毎回数え直す
		// （同じ前提クエストを何度クリアしても過大カウントされない＝最低一回クリアの要件を満たす）
		int32 ClearedCount = 0;
		for (const FName& ReqID : Data.RequiredQuestIDs)
		{
			if (EverCompletedQuestIDs.Contains(ReqID))
			{
				ClearedCount++;
			}
		}

		// RequiredAmountは使わず、RequiredQuestIDsを「全部」クリアすることを必須とする
		const int32 RequiredCount = Data.RequiredQuestIDs.Num();
		Progress.CurrentAmount = FMath::Min(ClearedCount, RequiredCount);
		bUpdatedAny = true;

		if (Progress.CurrentAmount >= RequiredCount)
		{
			Progress.Status = EQuestStatus::ObjectiveCleared; // 報告待ち状態へ

			if (OwnerChar)
			{
				FString LogMsg = FString::Printf(TEXT("クエスト「%s」の目的を達成した！"), *Data.QuestName.ToString());
				OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
			}

			if (USoundBase* Sound = ResolveQuestSound(Data.ObjectiveClearedSound, &AMyProject1HUD::DefaultQuestObjectiveClearedSound))
			{
				UGameplayStatics::PlaySound2D(GetWorld(), Sound);
			}
		}
		else if (OwnerChar)
		{
			FString LogMsg = FString::Printf(TEXT("実績クエスト「%s」の進捗（%d / %d）"), *Data.QuestName.ToString(), Progress.CurrentAmount, RequiredCount);
			OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::Default);
		}
	}

	if (bUpdatedAny)
	{
		OnQuestUpdated.Broadcast(NAME_None);
	}
}

void UQuestComponent::CheckInitialAchievementProgress(FName QuestID)
{
	FQuestData Data;
	if (!GetQuestData(QuestID, Data) || Data.QuestType != EQuestType::Achievement) return;

	for (FQuestProgress& Progress : ActiveQuests)
	{
		if (Progress.QuestID != QuestID || Progress.Status != EQuestStatus::InProgress) continue;

		int32 ClearedCount = 0;
		for (const FName& ReqID : Data.RequiredQuestIDs)
		{
			if (EverCompletedQuestIDs.Contains(ReqID))
			{
				ClearedCount++;
			}
		}

		// RequiredAmountは使わず、RequiredQuestIDsを「全部」クリアすることを必須とする
		const int32 RequiredCount = Data.RequiredQuestIDs.Num();
		Progress.CurrentAmount = FMath::Min(ClearedCount, RequiredCount);

		if (Progress.CurrentAmount >= RequiredCount)
		{
			Progress.Status = EQuestStatus::ObjectiveCleared;

			if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner()))
			{
				FString LogMsg = FString::Printf(TEXT("クエスト「%s」の目的を達成した！"), *Data.QuestName.ToString());
				OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
			}

			if (USoundBase* Sound = ResolveQuestSound(Data.ObjectiveClearedSound, &AMyProject1HUD::DefaultQuestObjectiveClearedSound))
			{
				UGameplayStatics::PlaySound2D(GetWorld(), Sound);
			}
		}
		break;
	}
}

void UQuestComponent::CheckInitialGatherProgress(FName QuestID)
{
	FQuestData Data;
	if (GetQuestData(QuestID, Data) && Data.QuestType == EQuestType::Gather)
	{
		AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
		UInventoryComponent* InvComp = OwnerChar ? OwnerChar->FindComponentByClass<UInventoryComponent>() : nullptr;

		if (InvComp)
		{
			// インベントリから現在の所持数を取得
			int32 CurrentCount = InvComp->GetItemQuantity(Data.TargetID);
			if (CurrentCount > 0)
			{
				// すでに持っていれば、その分だけカウントを進める
				UpdateGatherObjective(Data.TargetID, CurrentCount);
			}
		}
	}
}

void UQuestComponent::CheckQuestTimeLimits()
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance());
	if (!GameInst) return;

	const int32 Today = GameInst->TotalElapsedDays;
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());

	bool bAnyFailed = false;

	// 失敗したクエストをその場でActiveQuestsから抜くので、末尾から走査する
	for (int32 i = ActiveQuests.Num() - 1; i >= 0; --i)
	{
		FQuestProgress& Progress = ActiveQuests[i];

		// 「報告するまで」動作させる仕様なので、InProgressとObjectiveClearedの両方が対象
		if (Progress.Status != EQuestStatus::InProgress && Progress.Status != EQuestStatus::ObjectiveCleared)
		{
			continue;
		}

		FQuestData Data;
		if (!GetQuestData(Progress.QuestID, Data)) continue;

		// 期限なし（機能OFF）
		if (Data.TimeLimitDays <= 0) continue;

		// 旧セーブ救済：受注日が未記録（番兵-1）なら、今日を起点に付け直して今回は失敗させない
		if (Progress.AcceptedTotalDays < 0)
		{
			Progress.AcceptedTotalDays = Today;
			continue;
		}

		const int32 DaysPassed = Today - Progress.AcceptedTotalDays;
		if (DaysPassed < Data.TimeLimitDays) continue;

		// --- 強制失敗処理（放棄=CancelQuestと同じ後始末。CompletedQuests/EverCompletedには積まない＝条件を満たせば再受注可） ---
		const FName FailedID = Progress.QuestID;

		// 対象出現トリガー(ObjectiveClearedFlag)を残さない
		ClearObjectiveClearedFlag(Data);

		ActiveQuests.RemoveAt(i);
		bAnyFailed = true;

		if (OwnerChar)
		{
			const FString LogMsg = FString::Printf(TEXT("クエスト「%s」は期限切れで失敗した。"), *Data.QuestName.ToString());
			OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
		}

		// 強制失敗のペナルティ（プレイヤーのステータス変化）
		if (Data.FailurePenaltyStat != ETargetStat::None && Data.FailurePenaltyAmount != 0.0f)
		{
			ApplyFailurePenaltyStat(Data.FailurePenaltyStat, Data.FailurePenaltyExtraStatName, Data.FailurePenaltyAmount);
		}
	}

	if (bAnyFailed)
	{
		OnQuestUpdated.Broadcast(NAME_None); // 全体更新
	}
}

bool UQuestComponent::ApplyFailurePenaltyStat(ETargetStat Stat, FName ExtraStatName, float Amount) const
{
	AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(GetOwner());
	if (!OwnerChar) return false;

	// 参照(&)で受け取り、ここへの加算がそのまま本体のステータスに反映される（DialogComponentと同じ方式）
	FCharacterStats& Stats = OwnerChar->GetCharacterStats();
	FString StatName;

	switch (Stat)
	{
	case ETargetStat::Fame:      Stats.Fame += Amount;      StatName = TEXT("名声");   break;
	case ETargetStat::Favor:     Stats.Favor += Amount;     StatName = TEXT("好感度"); break;
	case ETargetStat::Hostility: Stats.Hostility += Amount; StatName = TEXT("敵対度"); break;
	case ETargetStat::Charm:     Stats.Charm += Amount;     StatName = TEXT("魅力");   break;
	case ETargetStat::Mental:    Stats.Mental += Amount;    StatName = TEXT("精神力"); break;
	case ETargetStat::Alcohol:   Stats.Alcohol += Amount;   StatName = TEXT("酒量");   break;
	case ETargetStat::STR:       Stats.STR += Amount;       StatName = TEXT("STR");    break;
	case ETargetStat::VIT:       Stats.VIT += Amount;       StatName = TEXT("VIT");    break;
	case ETargetStat::DEX:       Stats.DEX += Amount;       StatName = TEXT("DEX");    break;
	case ETargetStat::AGI:       Stats.AGI += Amount;       StatName = TEXT("AGI");    break;
	case ETargetStat::CustomExtraStat:
	{
		// 会話アクションのExtraStatと同じく、事前登録済みのキーしか変更しない（無ければ警告のみ）
		if (ExtraStatName.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("【Quest】強制失敗ペナルティでCustomExtraStatが指定されていますが、FailurePenaltyExtraStatNameが空です。"));
			return false;
		}
		float* CurrentVal = Stats.ExtraStats.Find(ExtraStatName);
		if (!CurrentVal)
		{
			UE_LOG(LogTemp, Warning, TEXT("【Quest】強制失敗ペナルティのExtraStat '%s' がプレイヤーに登録されていません。"), *ExtraStatName.ToString());
			return false;
		}
		*CurrentVal += Amount;
		StatName = ExtraStatName.ToString();
		break;
	}
	default:
		// AttackPower/DefensePower等の自動再計算されるステータスや未対応の値が指定された場合は
		// 黙って書き換えず、設定ミスとして警告だけ出す
		UE_LOG(LogTemp, Warning, TEXT("【Quest】強制失敗ペナルティに未対応のステータス指定 (%d) があります。名声・好感度・敵対度・魅力・精神力・酒量・STR/VIT/DEX/AGI・カスタムステータス のいずれかを指定してください。"), (int32)Stat);
		return false;
	}

	const FString Sign = (Amount > 0) ? TEXT("上がった") : TEXT("下がった");
	const FString LogMsg = FString::Printf(TEXT("%sが %.0f %s。"), *StatName, FMath::Abs(Amount), *Sign);
	OwnerChar->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
	OwnerChar->NotifyStatsChanged();

	return true;
}