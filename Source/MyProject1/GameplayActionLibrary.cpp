#include "GameplayActionLibrary.h"
#include "RpgCharacterInterface.h"
#include "QuestComponent.h"
#include "InventoryComponent.h"
#include "MyProject1GameInstance.h"
#include "MyProject1Character.h"
#include "WarpPortal.h"
#include "GameFramework/Character.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

void UGameplayActionLibrary::ExecuteAction(IRpgCharacterInterface* RpgInterface, AActor* OwnerActor, AActor* ContextActor, UWorld* World,
	EDialogActionType ActionType, const FString& ActionPayload, FName ItemID, int32 ItemAmount,
	EStatTargetActor AnimSequenceRowPlayTarget, const TSoftObjectPtr<USkeletalMesh>& AnimSequenceNPCMeshOverride,
	bool bHideContextActorDuringAnimEvent)
{
	if (!RpgInterface) return;

	switch (ActionType)
	{
	case EDialogActionType::AcceptQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->AcceptQuest(FName(*ActionPayload));
		break;

	case EDialogActionType::ReportQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->ReportQuest(FName(*ActionPayload));
		break;

	case EDialogActionType::CancelQuest:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
			QuestComp->CancelQuest(FName(*ActionPayload));
		break;

	case EDialogActionType::TalkProgress:
		if (UQuestComponent* QuestComp = RpgInterface->GetQuestComponent())
		{
			// 話しかけた相手（NPC）の識別はActorのTags（詳細パネルで設定）を使う。親クラスを問わず使えるようにするため
			QuestComp->UpdateTalkObjective(FName(*ActionPayload), ContextActor);
		}
		break;

	case EDialogActionType::AddFlag:
		RpgInterface->AddFlag(FName(*ActionPayload));
		break;

	case EDialogActionType::RemoveFlag:
		RpgInterface->RemoveFlag(FName(*ActionPayload));
		break;

	case EDialogActionType::Warp:
		if (World)
		{
			if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(World->GetGameInstance()))
			{
				FName WarpIDToUse = FName(*ActionPayload);

				if (ActionPayload.IsEmpty() || WarpIDToUse.IsNone())
				{
					if (AWarpPortal* Portal = Cast<AWarpPortal>(ContextActor))
					{
						WarpIDToUse = Portal->TargetWarpID;
					}
				}

				if (!WarpIDToUse.IsNone())
				{
					// ワープ関数はACharacterを要求するため、OwnerをACharacterにキャストして渡す
					if (ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor))
					{
						GameInst->RequestWarp(WarpIDToUse, OwnerChar);
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("ワープIDが設定されていません！"));
				}
			}
		}
		break;

	case EDialogActionType::RequestRankUp:
		// ActionPayloadに設定先の等級（EAdventurerRankの行名。例："Rank4"）を入れて使う
		RpgInterface->SetAdventurerRank(FName(*ActionPayload));
		break;

	case EDialogActionType::AddSkinOverlay:
		// ActionPayloadにタトゥー/傷跡のRowName（TattooDataTable等の行名）を入れて使う。
		// ショップ購入ではない強制追加のため、常にTattooカテゴリ・無償(bIsShopPurchase=false)で扱う
		if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(OwnerActor))
		{
			OwnerChar->TryAddSkinOverlay(FName(*ActionPayload), false);
		}
		break;

	case EDialogActionType::RemoveSkinOverlay:
		// AddSkinOverlayと対。ActionPayloadに消去対象のRowNameを入れて使う（常にTattooカテゴリ扱い）
		if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(OwnerActor))
		{
			OwnerChar->TryRemoveSkinOverlay(FName(*ActionPayload), false);
		}
		break;

	case EDialogActionType::EquipEquipment:
		// ActionPayloadにDT_Equipments（EquipmentDataTable）の行名を入れる。プレイヤー自身（OwnerActor）に装備させる
		if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(OwnerActor))
		{
			if (OwnerChar->EquipmentDataTable)
			{
				const FName EquipRowName(*ActionPayload);
				if (FEquipmentData* EquipData = OwnerChar->EquipmentDataTable->FindRow<FEquipmentData>(EquipRowName, TEXT("ExecuteAction EquipEquipment")))
				{
					OwnerChar->EquipItem(EquipRowName, *EquipData);
				}
			}
		}
		break;

	case EDialogActionType::UnequipEquipment:
		// ActionPayloadにEEquipmentSlotの行名（例："Torso"）を入れる。プレイヤー自身（OwnerActor）の装備を、
		// ロックの有無に関わらず強制的に外し、インベントリへ戻す
		if (AMyProject1Character* OwnerChar = Cast<AMyProject1Character>(OwnerActor))
		{
			const UEnum* SlotEnum = StaticEnum<EEquipmentSlot>();
			const int64 SlotValue = SlotEnum->GetValueByName(FName(*ActionPayload));
			if (SlotValue != INDEX_NONE)
			{
				OwnerChar->UnequipItemAndReturnToInventory(static_cast<EEquipmentSlot>(SlotValue));
			}
		}
		break;

	case EDialogActionType::AddItem:
		// ItemID/ItemAmountで指定したアイテムをプレイヤーのインベントリに追加する（渡す）。
		// カバンが満杯で入り切らない場合はAddItem側がfalseを返すが、ここでは通知は出さない
		if (!ItemID.IsNone() && ItemAmount > 0)
		{
			if (UInventoryComponent* Inv = OwnerActor ? OwnerActor->FindComponentByClass<UInventoryComponent>() : nullptr)
				Inv->AddItem(ItemID, ItemAmount);
		}
		break;

	case EDialogActionType::RemoveItem:
		// ItemID/ItemAmountで指定したアイテムをプレイヤーのインベントリから削除する（奪う）。
		// UInventoryComponent::RemoveItem自体はログを出さない（ポーション消費など内部利用で無言にするため）ので、
		// 会話でのアイテム受け渡し時だけここでログを出す（AddItem側はAddItem内でログが出るのと対になる）
		if (!ItemID.IsNone() && ItemAmount > 0)
		{
			if (UInventoryComponent* Inv = OwnerActor ? OwnerActor->FindComponentByClass<UInventoryComponent>() : nullptr)
			{
				if (Inv->RemoveItem(ItemID, ItemAmount))
				{
					FItemData ItemInfo;
					const FString ItemName = Inv->GetItemDataBP(ItemID, ItemInfo) ? ItemInfo.Name : ItemID.ToString();
					const FString LogMsg = (ItemAmount == 1)
						? FString::Printf(TEXT("%sを渡した。"), *ItemName)
						: FString::Printf(TEXT("%sを%d個渡した。"), *ItemName, ItemAmount);
					RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
				}
			}
		}
		break;

	case EDialogActionType::AddGil:
		// ActionPayloadに入れた金額（￥）の数値文字列をプレイヤーの所持金に加算する（報酬の前金など）。
		// RemoveItemと同じく、渡した時だけここでシステムログを出す
		{
			const int32 GilAmount = FCString::Atoi(*ActionPayload);
			if (GilAmount > 0)
			{
				if (UInventoryComponent* Inv = OwnerActor ? OwnerActor->FindComponentByClass<UInventoryComponent>() : nullptr)
				{
					Inv->AddGil(GilAmount);
					RpgInterface->OnReceiveLogMessage(FString::Printf(TEXT("%d￥ 手に入れた。"), GilAmount), ELogMessageType::System);
				}
			}
		}
		break;

	case EDialogActionType::Close:
		// ダイアログUIを閉じる処理は呼び出し側（DialogComponent）が個別に行う。ここでは何もしない
		break;

	case EDialogActionType::PlayAnimSequence:
		// ActionPayloadにDT_AnimEventsの行名（AnimEventID）を入れる。イベント抽選・ワープを経由せず直接再生する。
		// ContextActor（話しかけている相手のNPC）はFAnimEventStep::PlayTarget=NPC時の再生対象になる。
		// AnimSequenceNPCMeshOverrideは、このAnimEventID再生中にスポーンするExtra参加者（ExtraPairings）の
		// メッシュ差し替えに使う（ContextActor自身のメッシュには影響しない）
		if (World)
		{
			if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(World->GetGameInstance()))
			{
				if (ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor))
				{
					// ワープを経由しない直接呼び出しのため、開始前にも暗転を挟む（bFadeInBeforeStart=true）
					GameInst->PlayAnimSequenceEvent(FName(*ActionPayload), OwnerChar, ContextActor, true, AnimSequenceNPCMeshOverride, bHideContextActorDuringAnimEvent);
				}
			}
		}
		break;

	case EDialogActionType::PlayAnimSequenceRow:
		// ActionPayloadにDT_AnimSequencesの行名を直接入れる。DT_AnimEvents・Tag抽選・BGM切替・暗転演出を経由せず、
		// 行のExtraPairingsも含めて1行分をそのまま再生する（位置調整の確認用）
		if (World)
		{
			if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(World->GetGameInstance()))
			{
				if (ACharacter* OwnerChar = Cast<ACharacter>(OwnerActor))
				{
					GameInst->PlayAnimSequenceRowDirect(FName(*ActionPayload), OwnerChar, ContextActor, AnimSequenceRowPlayTarget);
				}
			}
		}
		break;

	default:
		break;
	}
}

void UGameplayActionLibrary::ApplyStatChange(IRpgCharacterInterface* RpgInterface, AActor* ContextActor,
	ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount)
{
	if (!RpgInterface || StatToChange == ETargetStat::None || StatChangeAmount == 0.0f) return;

	float ChangeVal = StatChangeAmount;
	FString StatName = TEXT("不明なステータス");

	// StatTargetActor=NPCなら、対象自身のMyStats（個体ごとのFavor/Hostility等）を書き換える。
	// キャストに失敗した場合（対象未設定、IRpgCharacterInterface非対応など）はRpgInterface側にフォールバックする
	IRpgCharacterInterface* StatOwnerInterface = RpgInterface;
	if (StatTargetActor == EStatTargetActor::NPC)
	{
		if (IRpgCharacterInterface* NPCInterface = Cast<IRpgCharacterInterface>(ContextActor))
		{
			StatOwnerInterface = NPCInterface;
		}
	}

	// 参照(&)で受け取るため、ここで書き換えると本体のステータスに直結します
	FCharacterStats& Stats = StatOwnerInterface->GetCharacterStats();

	switch (StatToChange)
	{
	case ETargetStat::Favor:
		Stats.Favor += ChangeVal;
		StatName = TEXT("好感度");
		break;
	case ETargetStat::Hostility:
		Stats.Hostility += ChangeVal;
		StatName = TEXT("敵対度");
		break;
	case ETargetStat::Fame:
		Stats.Fame += ChangeVal;
		StatName = TEXT("名声");
		break;
	case ETargetStat::Charm:
		Stats.Charm += ChangeVal;
		StatName = TEXT("魅力");
		break;

	case ETargetStat::Alcohol:
		Stats.Alcohol += ChangeVal;
		StatName = TEXT("酒量");
		break;

	case ETargetStat::Mental:
		Stats.Mental += ChangeVal;
		StatName = TEXT("精神力");
		break;

	case ETargetStat::CustomExtraStat:
		if (!ExtraStatName.IsNone())
		{
			float* CurrentVal = Stats.ExtraStats.Find(ExtraStatName);

			if (CurrentVal)
			{
				*CurrentVal += ChangeVal;
				StatName = Stats.GetExtraStatDisplayName(ExtraStatName);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("【GameplayAction Error】 ExtraStat '%s' が登録されていません！"), *ExtraStatName.ToString());
				ChangeVal = 0.0f;
			}
		}
		else
		{
			ChangeVal = 0.0f;
		}
		break;
	default:
		break;
	}

	if (ChangeVal != 0.0f)
	{
		FString Sign = (ChangeVal > 0) ? TEXT("上がった") : TEXT("下がった");
		FString LogMsg = FString::Printf(TEXT("%sが %.0f %s。"), *StatName, FMath::Abs(ChangeVal), *Sign);

		// ログもUI通知もインターフェース経由
		RpgInterface->OnReceiveLogMessage(LogMsg, ELogMessageType::System);
		RpgInterface->NotifyStatsChanged();
	}
}

bool UGameplayActionLibrary::TryGetTargetStatValue(IRpgCharacterInterface* RpgInterface, AActor* ContextActor,
	ETargetStat TargetStat, EStatTargetActor StatTargetActor, FName ExtraStatName, float& OutValue)
{
	OutValue = 0.0f;
	if (!RpgInterface || TargetStat == ETargetStat::None) return false;

	// StatTargetActor=NPCなら、対象自身のMyStats（個体ごとのFavor/Hostility等）を読み取る。
	// ApplyStatChangeと異なり、対象未設定/非対応の場合はRpgInterfaceへフォールバックせず判定不能（false）とする
	IRpgCharacterInterface* StatOwnerInterface = RpgInterface;
	if (StatTargetActor == EStatTargetActor::NPC)
	{
		IRpgCharacterInterface* NPCInterface = Cast<IRpgCharacterInterface>(ContextActor);
		if (!NPCInterface) return false;
		StatOwnerInterface = NPCInterface;
	}

	const FCharacterStats& Stats = StatOwnerInterface->GetCharacterStats();

	switch (TargetStat)
	{
	case ETargetStat::HP:                  OutValue = Stats.HP; return true;
	case ETargetStat::STR:                 OutValue = Stats.STR; return true;
	case ETargetStat::DEX:                 OutValue = Stats.DEX; return true;
	case ETargetStat::VIT:                 OutValue = Stats.VIT; return true;
	case ETargetStat::AGI:                 OutValue = Stats.AGI; return true;
	case ETargetStat::Stamina:             OutValue = Stats.Stamina; return true;
	case ETargetStat::Accuracy:            OutValue = Stats.Accuracy; return true;
	case ETargetStat::Evasion:             OutValue = Stats.Evasion; return true;
	case ETargetStat::AttackPower:         OutValue = Stats.AttackPower; return true;
	case ETargetStat::DefensePower:        OutValue = Stats.DefensePower; return true;
	case ETargetStat::Favor:               OutValue = Stats.Favor; return true;
	case ETargetStat::Hostility:           OutValue = Stats.Hostility; return true;
	case ETargetStat::Fame:                OutValue = Stats.Fame; return true;
	case ETargetStat::Charm:               OutValue = Stats.Charm; return true;
	case ETargetStat::Alcohol:             OutValue = Stats.Alcohol; return true;
	case ETargetStat::Mental:              OutValue = Stats.Mental; return true;
	case ETargetStat::FatigueGainRate:     OutValue = Stats.FatigueGainRateBonus; return true;
	case ETargetStat::FatigueRecoveryRate: OutValue = Stats.FatigueRecoveryRateBonus; return true;
	case ETargetStat::OGaugeGainRate:      OutValue = Stats.OGaugeGainRateBonus; return true;
	case ETargetStat::OGaugeRecoveryRate:  OutValue = Stats.OGaugeRecoveryRateBonus; return true;
	case ETargetStat::MovementSpeedRate:   OutValue = Stats.MovementSpeedRateBonus; return true;
	case ETargetStat::CustomExtraStat:
		if (!ExtraStatName.IsNone())
		{
			if (const float* Extra = Stats.ExtraStats.Find(ExtraStatName))
			{
				OutValue = *Extra;
				return true;
			}
		}
		return false;
	default:
		// MP等、FCharacterStatsに対応するメンバーが存在しないステータス種別は未対応
		return false;
	}
}
