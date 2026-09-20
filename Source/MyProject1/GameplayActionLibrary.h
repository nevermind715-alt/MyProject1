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
	 *  ContextActorは、TalkProgressの対象特定・Warpの起点ポータル（AWarpPortal）判定に使う（不要ならnullptr可）。 */
	static void ExecuteAction(IRpgCharacterInterface* RpgInterface, AActor* OwnerActor, AActor* ContextActor, UWorld* World,
		EDialogActionType ActionType, const FString& ActionPayload, FName ItemID, int32 ItemAmount);

	/** StatToChange/StatChangeAmountによるステータス変化本体。ActionTypeとは独立しており、両方が設定されていれば
	 *  ExecuteActionと併用して両方適用される（FDialogChoice/FEventActionの元々の仕様と同じ）。
	 *  StatTargetActor=NPC時はContextActorをIRpgCharacterInterfaceとして扱う（対応していなければRpgInterfaceにフォールバック）。 */
	static void ApplyStatChange(IRpgCharacterInterface* RpgInterface, AActor* ContextActor,
		ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount);
};
