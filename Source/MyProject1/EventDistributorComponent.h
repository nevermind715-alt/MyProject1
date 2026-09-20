#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "MyProject1Types.h"
#include "EventDistributorComponent.generated.h"

/**
 * NPC/QuestItemPoint/敵キャラクターなど「プレイヤーにイベントを起こすきっかけ」となるActorへ付ける汎用コンポーネント。
 * EventPoolDataTable（DT_EventPools）のEventPoolID行を重み付き抽選し、当選したイベント（DT_EventDefinitionsの行）を
 * UMyProject1GameInstance::StartEventへ渡して開始する（施設へのワープ・成立条件の監視は全てGameInstance側が行う）。
 *
 * 呼び出し元は3経路を想定：
 * - 会話：EDialogActionType::TriggerEvent（ActionPayload=EventPoolID）から、話しかけたNPC側のこのコンポーネントを呼ぶ
 * - QuestItemPoint：TryInteract成功時に、同じActorのこのコンポーネントを呼ぶ
 * - 戦闘敗北：プレイヤーが敵にHP0にされた際、その敵側のこのコンポーネントを呼ぶ（AMyProject1Character::TakeDamage参照）
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UEventDistributorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEventDistributorComponent();

	/** 抽選プール（DT_EventPools）のデータテーブル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	UDataTable* EventPoolDataTable;

	/** EventPoolDataTable内の抽選プール行名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FName EventPoolID;

	/** EventPoolIDのプールから重み付き抽選を行い、当選したイベントを開始する（施設へワープする）。
	 *  PlayerCharacterは実際にイベントを体験させる対象（会話・ItemPointの場合は操作しているプレイヤー、
	 *  戦闘敗北の場合はHP0にされたプレイヤー） */
	UFUNCTION(BlueprintCallable, Category = "Event")
	void TriggerEventPool(class AMyProject1Character* PlayerCharacter);
};
