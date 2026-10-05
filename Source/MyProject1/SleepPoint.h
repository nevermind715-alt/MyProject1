#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SleepPoint.generated.h"

/**
 * ベッドや布団に置く、睡眠用のインタラクトポイント。
 * QuestItemPointと同じ「Tagsに"NPC"を付けてInteractRange判定に乗せる」方式で、
 * インタラクトされたら既存のTryOpenTimeSkipMenu(true)を呼んで睡眠メニュー(WBP_TimeSkipMenu)を開くだけの窓口。
 * 時間を進める処理・暗転・UIは全て既存の仕組み（WBP_TimeSkipMenu/GameInstance）に任せる。
 */
UCLASS()
class MYPROJECT1_API ASleepPoint : public AActor
{
	GENERATED_BODY()

public:
	ASleepPoint();

	// --- 見た目 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* Mesh;

	/** trueなら、このベッドで実際に眠り終えた時（時間スキップ完了時）に、このActorへ追加した
	 *  UEventDistributorComponentの抽選を発生させる（金品を盗まれる等）。falseなら何も起きない。
	 *  EventDistributorComponent自体はエディタでこのActor（BP_SleepPoint）にコンポーネントとして追加し、
	 *  EventPoolID等を設定しておくこと */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bTriggerEventPoolOnSleep = false;

	/** 抽選で何らかのイベント（EventID空欄以外）が当選した場合、まだ画面が真っ暗なこのタイミングで表示する
	 *  暗転セリフ。既存の暗転セリフUI（ActionType=ShowTextDuringFadeが使っているのと同じUDialogComponent::
	 *  OnFadeNarrationLine/OnFadeNarrationClosed表示）をそのまま流用するが、DT_Dialogs（会話データ）やTryStartDialog
	 *  （会話開始フロー）は経由しないので、ここに直接セリフを書く。クリック/決定入力での読み進めは行わず、
	 *  EventEncounterNarrationDisplaySeconds秒間表示した後、自動的に消えてイベントを発動する。
	 *  空欄なら何も表示せず、抽選結果のイベントをすぐに発動する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (MultiLine = true))
	FText EventEncounterNarrationText;

	/** EventEncounterNarrationTextを表示しておく秒数（この秒数後、自動的に消えてイベントを発動する） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", meta = (ClampMin = "0.1"))
	float EventEncounterNarrationDisplaySeconds = 3.0f;

	/** プレイヤーがインタラクトした時に呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "Sleep")
	void TryInteract(class AMyProject1Character* Interactor);

	/** TryInteractが成功した（睡眠メニューを開けた）時点のプレイヤー座標。
	 *  睡眠イベント（bTriggerEventPoolOnSleep）がFEventDefinition::bUseContextActorLocationInsteadOfWarpで
	 *  このSleepPoint自身へワープした場合、イベント成立/失敗後の戻り先としてUMyProject1GameInstance::
	 *  ResolveActiveEventが参照する（DT_EventDefinitionsのReturnWarpIDは使わない） */
	FTransform GetPreSleepInteractTransform() const { return PreSleepInteractTransform; }

	/** 睡眠による時間スキップが完了した直後（まだ画面が真っ暗な状態）にUMyProject1GameInstance::ExecuteWarpProcess
	 *  から呼ばれる。bTriggerEventPoolOnSleepがtrueで、このActorにUEventDistributorComponentが付いていれば、
	 *  その抽選だけを行い、結果をPendingSleepEventIDへ記憶する（この時点ではまだ発動しない）。
	 *  戻り値は、実際に何らかのイベントが当選したかどうか（「何も起きない」用の空欄EventIDが当選した場合、
	 *  bTriggerEventPoolOnSleepがfalseの場合、コンポーネント自体が付いていない場合はfalse） */
	bool TriggerSleepEventPoolIfEnabled(class AMyProject1Character* PlayerCharacter);

	/** TriggerSleepEventPoolIfEnabledがtrueを返した場合、GameInstance側で明転タイマーの自動開始を止めた直後に
	 *  呼ばれる。EventEncounterNarrationTextが設定されていれば（まだ真っ暗な画面のまま）
	 *  UMyProject1GameInstance::BeginSleepEventNarrationでそれを表示し、読み終えたタイミングで
	 *  OnSleepEventNarrationFinishedを呼んでもらう。未設定なら次のTickでイベントを発動する */
	void BeginPendingSleepEvent(class AMyProject1Character* PlayerCharacter);

	/** UMyProject1GameInstance::HandleSleepEventNarrationTimerCompleteが、EventEncounterNarrationDisplaySeconds
	 *  経過時に呼ぶ。BeginPendingSleepEventで立てた入力ロックを解除し、実際にイベントを発動する */
	void OnSleepEventNarrationFinished(class AMyProject1Character* PlayerCharacter);

private:
	/** TriggerSleepEventPoolIfEnabledが抽選した結果（BeginPendingSleepEvent/StartPendingSleepEvent実行待ち） */
	FName PendingSleepEventID;

	/** GetPreSleepInteractTransform用。TryInteract成功時に記憶する */
	FTransform PreSleepInteractTransform;

	/** PendingSleepEventIDに記憶されている抽選結果を実際に発動する（GameInstance::StartEventを呼ぶ）。
	 *  StartEvent完了後、ClearCondition=Instant等でGameInstance::bHasActiveEventがfalseのまま（＝ワープ等を
	 *  経由しなかった）場合は、ここで明転を再開する（GameInstance::ResumeFadeInAfterSleepEvent）。
	 *  Warp/AnimationSequence等を経由した場合はbHasActiveEventがtrueになり、それ以降のフェード管理は
	 *  既存のResolveActiveEvent側に委ねるため、ここでは何もしない */
	void StartPendingSleepEvent(class AMyProject1Character* PlayerCharacter);
};
