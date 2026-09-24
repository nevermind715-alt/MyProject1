#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyProject1Types.h"
#include "GameplayActionLibrary.generated.h"

class IRpgCharacterInterface;

/**
 * UDialogComponent::ExecuteActionCoreと、イベント分岐システム（UMyProject1GameInstance::ResolveActiveEvent）の
 * 両方から呼ばれる、ActionType 1つ分の実行本体を共通化したもの。
 * 元はUDialogComponent::ExecuteActionCore内のswitch文とStatToChangeブロックだったものをそのまま切り出している。
 */
UCLASS()
class MYPROJECT1_API UGameplayActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** ActionTypeのswitch本体。EDialogActionType::Closeはダイアログ専用（会話UIを閉じる）のためここでは何もしない。
	 *  呼び出し側がActionType==Closeを個別に判定して対応すること（DialogComponent::ExecuteActionCoreを参照）。
	 *  ContextActorは、TalkProgressの対象特定・Warpの起点ポータル（AWarpPortal）判定・
	 *  ActionType=PlayAnimSequenceRow時のNPC側再生対象に使う（不要ならnullptr可）。
	 *  AnimSequenceRowPlayTargetはActionType=PlayAnimSequenceRow専用（FDialogChoice::AnimSequenceRowPlayTarget参照）。
	 *  AnimSequenceNPCMeshOverrideはActionType=PlayAnimSequence専用（FDialogData::AnimSequenceNPCMeshOverride参照）。
	 *  設定されていれば、再生するDT_AnimEventsがスポーンするExtra参加者（ExtraPairings）のメッシュを
	 *  DT_AnimSequences側のPairing.Meshの代わりにこれで差し替える（ContextActor自身のメッシュには影響しない）。
	 *  bHideContextActorDuringAnimEventはActionType=PlayAnimSequence専用（FDialogChoice::bHideTalkingNPCDuringAnimEvent参照）。
	 *  trueならAnimEvent再生中だけContextActor自身を非表示にする */
	static void ExecuteAction(IRpgCharacterInterface* RpgInterface, AActor* OwnerActor, AActor* ContextActor, UWorld* World,
		EDialogActionType ActionType, const FString& ActionPayload, FName ItemID, int32 ItemAmount,
		EStatTargetActor AnimSequenceRowPlayTarget = EStatTargetActor::Player,
		const TSoftObjectPtr<class USkeletalMesh>& AnimSequenceNPCMeshOverride = nullptr,
		bool bHideContextActorDuringAnimEvent = false);

	/** StatToChange/StatChangeAmountによるステータス変化本体。ActionTypeとは独立しており、両方が設定されていれば
	 *  ExecuteActionと併用して両方適用される（FDialogChoice/FEventActionの元々の仕様と同じ）。
	 *  StatTargetActor=NPC時はContextActorをIRpgCharacterInterfaceとして扱う（対応していなければRpgInterfaceにフォールバック）。 */
	static void ApplyStatChange(IRpgCharacterInterface* RpgInterface, AActor* ContextActor,
		ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount);

	/** FDialogChoice::RequiredStat等、選択肢の表示条件判定に使う読み取り専用ヘルパー（ApplyStatChangeの読み取り版）。
	 *  StatTargetActor=NPC時はContextActorをIRpgCharacterInterfaceとして扱う（対応していなければfalseを返す）。
	 *  TargetStat==None、FCharacterStatsに対応する値がないステータス種別、CustomExtraStatで未登録のExtraStatNameの
	 *  場合はfalseを返す（呼び出し側は安全側として「選択不可」として扱うこと）。 */
	static bool TryGetTargetStatValue(IRpgCharacterInterface* RpgInterface, AActor* ContextActor,
		ETargetStat TargetStat, EStatTargetActor StatTargetActor, FName ExtraStatName, float& OutValue);
};
