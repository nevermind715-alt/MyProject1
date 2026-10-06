#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "MyProject1Types.h"
#include "EventDistributorComponent.generated.h"

/** 戦闘敗北（このコンポーネントの持ち主の敵にプレイヤーがHP0にされた）経由のイベント発動／ワープリスタートの直後に通知する */
DECLARE_MULTICAST_DELEGATE(FOnDefeatedPlayerSignature);

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

	/** TriggerEventPool呼び出し側がExtraParticipantMeshOverride引数を指定しなかった場合（未設定=nullptr）に、
	 *  代わりに使うメッシュ。ANPCSpawner::bOverrideExtraParticipantMeshWithOwnMeshが有効な場合、スポーン時に
	 *  この敵自身のジョブメッシュ（DT_Jobs::FJobAttributes::CharacterMesh）がここへ自動的にセットされる。
	 *  未設定（null）ならAnimEventID側のFAnimEventPairingで個別に設定されたメッシュがそのまま使われる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	TSoftObjectPtr<class USkeletalMesh> ExtraParticipantMeshOverrideDefault;

	/** trueの場合、TriggerEventPool呼び出し側がbHideContextActorDuringAnimEventを指定しなくても（戦闘敗北経由等）、
	 *  AnimEvent再生中だけこのコンポーネントの持ち主（敵自身）を非表示にする。ExtraPairingsでスポーンする
	 *  AAnimEventActorと本体が重なって見えるのを防ぐ。ANPCSpawner::bHideEnemyDuringAnimEventから設定される想定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bHideOwnerDuringAnimEventDefault = false;

	/** 戦闘敗北によるTriggerEventPool呼び出し時のみ使用。WarpToRestartIDが空欄でない場合、この確率（%）の
	 *  抽選でイベントディストリビュータを起動するかどうかを決める（AMyProject1Character::TakeDamage参照）。
	 *  抽選に外れた場合はイベント抽選を行わず、代わりにWarpToRestartIDへワープする。
	 *  WarpToRestartIDが空欄ならこの値は使われず、常に従来通りイベントディストリビュータを起動する。
	 *  初期値100＝常にイベントディストリビュータを起動（従来動作のまま）。
	 *  ANPCSpawner::EventDistributorChance / SpawnerOverrides|Eventから上書きされる想定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float EventDistributorChance = 100.0f;

	/** 空欄でなければ、戦闘敗北時にEventDistributorChance(%)の抽選に外れた場合、イベントディストリビュータを
	 *  起動せずこのWarpID（DT_WarpDestinationsの行名。町の治療院など）へワープしてリスタートさせる。
	 *  ANPCSpawner::WarpToRestartID / SpawnerOverrides|Eventから上書きされる想定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FName WarpToRestartID;

	/** 戦闘敗北経由のTriggerEventPool呼び出し（または抽選外れのWarpToRestartID）の直後に呼ばれる。
	 *  ANPCSpawner::bRemoveOnPlayerDefeat等が、敗北後の敵・フラグ用ポイントの削除とクエスト失敗のために購読する
	 *  （AMyProject1Character::TakeDamage参照。Blueprint非公開のC++専用デリゲート） */
	FOnDefeatedPlayerSignature OnDefeatedPlayer;

	/** EventPoolIDOverrideが指定されていればそちらを、Noneならこのコンポーネント自身のEventPoolIDを使い、
	 *  そのプールから重み付き抽選を行って当選したイベントを開始する（施設へワープする）。
	 *  PlayerCharacterは実際にイベントを体験させる対象（会話・ItemPointの場合は操作しているプレイヤー、
	 *  戦闘敗北の場合はHP0にされたプレイヤー）。
	 *  EventPoolIDOverrideは、同じNPCの会話（DT_Dialogsの選択肢/セリフ単体アクション）ごとに違うプールを
	 *  引きたい場合用（ActionPayload=EventPoolID）。QuestItemPoint・戦闘敗北経由は未使用のためNoneのまま呼ぶ。
	 *  ExtraParticipantMeshOverrideが未設定（null）の場合、ExtraParticipantMeshOverrideDefaultが設定されていれば
	 *  そちらを使う（戦闘敗北経由等、呼び出し側がメッシュを指定しないケース向け。ANPCSpawner参照）。
	 *  ExtraParticipantMeshOverrideが設定されていれば、当選したイベントがClearCondition=AnimationSequenceで
	 *  再生するAnimEventIDのExtra参加者（ExtraPairings）のメッシュをこれで差し替える
	 *  （FDialogChoice::AnimSequenceNPCMeshOverride参照。会話経由以外は未使用のためnullptrのまま呼ぶ）。
	 *  bHideContextActorDuringAnimEventが設定されていれば、そのAnimEvent再生中だけこのコンポーネントの
	 *  OwnerActor（話しかけた相手のNPC自身）を非表示にする（FDialogChoice::bHideTalkingNPCDuringAnimEvent参照）。
	 *  戻り値：EventIDが空欄でない候補が当選し、実際にStartEventを呼んだ場合はtrue。抽選自体が行えなかった場合、
	 *  または「何も起きない」用の空欄EventID候補が当選した場合はfalse（ASleepPoint::TriggerSleepEventPoolIfEnabled等、
	 *  「何かが起きたかどうか」で分岐したい呼び出し元向け） */
	UFUNCTION(BlueprintCallable, Category = "Event")
	bool TriggerEventPool(class AMyProject1Character* PlayerCharacter, FName EventPoolIDOverride = NAME_None, const TSoftObjectPtr<class USkeletalMesh>& ExtraParticipantMeshOverride = nullptr, bool bHideContextActorDuringAnimEvent = false);

	/** TriggerEventPoolと同じ重み付き抽選だけを行い、StartEventは呼ばない。当選したEventID
	 *  （DT_EventDefinitionsの行名）を返し、「何も起きない」用の空欄EventID候補が当選した場合、
	 *  または抽選自体が行えなかった場合は空欄（NAME_None）を返す。
	 *  ASleepPoint（睡眠イベント）等、抽選結果の判定と実際の発動タイミングを分離したい呼び出し元向け */
	UFUNCTION(BlueprintCallable, Category = "Event")
	FName RollEventPool(class AMyProject1Character* PlayerCharacter, FName EventPoolIDOverride = NAME_None) const;
};
