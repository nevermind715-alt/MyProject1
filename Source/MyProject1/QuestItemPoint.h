#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Types.h"
#include "QuestItemPoint.generated.h"

// このアクタでアイテムを渡す（Acquire）か回収する（Absorb）か、アイテムを一切介さずインタラクトのみ扱う（InteractOnly）かの選択
UENUM(BlueprintType)
enum class EItemInteractMode : uint8
{
	Acquire      UMETA(DisplayName = "取得させる（渡す）"),
	Absorb       UMETA(DisplayName = "回収する（奪う/消費させる）"),
	InteractOnly UMETA(DisplayName = "アイテムなし（インタラクトのみ）")
};

/**
 * レベルに置いて、インタラクトするとアイテムを渡す/回収する汎用ギミック。
 * 「お供え物をする」「ゴミを拾う」といったお使いクエストの目的地として使う想定。
 *
 * - Acquireモードは InventoryComponent::AddItem が内部で QuestComponent::UpdateGatherObjective を
 *   自動的に呼ぶため、収集(Gather)クエストと特別な連携コードなしにそのまま連動する。
 * - Absorbモードで TalkProgressQuestID を設定すると、成功時に QuestComponent::UpdateTalkObjective を呼ぶ。
 *   これはNPCと会話した時と全く同じ経路（Delivery型クエストのRequiredTalkNPCIDsが見るのはNPCではなく
 *   AActor*とTagsの一致だけ）なので、このアクタ自身のTagsにRequiredTalkNPCIDsと同じ名前を1つ追加しておけば、
 *   「話す」代わりに「供える」でクエストチェーンを進められる。
 * - InteractOnlyモードはアイテムのやり取りを一切せず、インタラクトした時点で常に成功扱いになる。
 *   墓に手を合わせる、神社にお参りする、といった「アイテムを介さないが台詞とクエスト進捗だけ発生させたい」場面向け。
 */
UCLASS()
class MYPROJECT1_API AQuestItemPoint : public AActor
{
	GENERATED_BODY()

public:
	AQuestItemPoint();

protected:
	virtual void BeginPlay() override;

public:
	// --- 見た目 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* Mesh;

	// --- アイテム設定 ---
	/** 取得させるか、回収するか、アイテムなしのインタラクトのみか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact")
	EItemInteractMode Mode = EItemInteractMode::Acquire;

	/** 対象アイテム（DT_ItemDataの行名）。空欄（None）ならアイテムのやり取りをせず、お金だけのポイントにできる。InteractOnlyモードでは未使用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact", meta = (EditCondition = "Mode != EItemInteractMode::InteractOnly"))
	FName ItemID;

	/** 個数。InteractOnlyモードでは未使用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact", meta = (ClampMin = "1", EditCondition = "Mode != EItemInteractMode::InteractOnly"))
	int32 Amount = 1;

	/** 増減するお金（ギル）。Acquireなら渡す、Absorbなら回収する。0ならお金のやり取りなし。InteractOnlyモードでは未使用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact", meta = (ClampMin = "0", EditCondition = "Mode != EItemInteractMode::InteractOnly"))
	int32 GilAmount = 0;

	// --- ログ ---
	/** インタラクト成功時にログへ流す台詞（空欄なら何も流さない） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact|Log")
	FString SuccessLogText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact|Log")
	ELogMessageType SuccessLogType = ELogMessageType::System;

	/** 失敗時（Absorbでアイテム不足/カバンが満杯/使用済み）に流す台詞。空欄なら既定の文言を使う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact|Log")
	FString FailureLogText;

	// --- クエスト連携 ---
	/** 空欄でなければ、成功時にこのQuestIDのUpdateTalkObjectiveを呼ぶ（Delivery型クエストの「話した」扱いにする）。
	    このアクタのTagsに、そのクエストのRequiredTalkNPCIDsと同じ名前を1つ追加しておくこと */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact|Quest")
	FName TalkProgressQuestID;

	/** 空欄でなければ、プレイヤーがこのフラグを持つまで非表示・当たり判定OFFにしておく
	    （NPCと会話してAddFlagされるまでこの供物台/回収ポイントの存在自体を隠したい時に使う） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact|Quest")
	FName RequiredFlag;

	// --- 使用回数 ---
	/** trueなら成功後に使用済みとなり、以後インタラクトできなくなる（ゴミ拾いなど）。
	    falseなら何度でも使える（賽銭箱など） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact")
	bool bOneTimeUse = false;

	/** 使用済みかどうか（bOneTimeUse時のみ意味を持つ） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Interact")
	bool bUsed = false;

	// --- 演出 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Interact|Audio")
	class USoundBase* InteractSound = nullptr;

	/** プレイヤーがインタラクトした時に呼ぶ */
	UFUNCTION(BlueprintCallable, Category = "Item Interact")
	void TryInteract(class AMyProject1Character* Interactor);

private:
	/** RequiredFlagの所持状況に応じて、表示/当たり判定のON・OFFを切り替える */
	void UpdateFlagVisibility(const class AMyProject1Character* PlayerChar);

	/** プレイヤーがフラグを獲得した時に呼ばれる（OnFlagAddedの購読先）。RequiredFlag一致時のみ表示を更新する */
	UFUNCTION()
	void OnPlayerFlagAdded(FName FlagName);
};
