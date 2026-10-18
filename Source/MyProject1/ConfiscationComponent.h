#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h"
#include "ConfiscationComponent.generated.h"

/**
 * プレイヤーの装備とカバンの中身を「没収」して保管し、後から「返却」するコンポーネント。
 * 捕まって留置所に送られ、囚人服だけ着せられて持ち物を取り上げられる、といったイベント用。
 *
 * ・没収の条件は DT_ConfiscationProfiles（FConfiscationProfile）の1行で決める。
 * ・没収品はStashID（保管先ID）ごとにStashesへ入る。セーブとレベル移動（スナップショット）で保持される。
 * ・トリガーは EDialogActionType::ConfiscateBelongings / ReturnBelongings（会話・イベント・QuestItemPointのアクションから呼べる）。
 *   箱から取り返す場合は、箱をAQuestItemPointで作り、成功率つき抽選の成功アクションにReturnBelongingsを指定する。
 * ・装備の着脱はAMyProject1Characterの既存関数（UnequipItem/EquipItem）、カバンの増減はUInventoryComponentの既存関数を使う。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UConfiscationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UConfiscationComponent();

	/** 没収条件のデータテーブル（RowStruct = FConfiscationProfile）。BP_Player側のこのコンポーネントに設定する */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Confiscation")
	UDataTable* ProfileDataTable = nullptr;

	/** 保管中の没収品（StashID → 中身）。セーブデータに保存される */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Confiscation")
	TMap<FName, FConfiscatedStash> Stashes;

	/** ProfileDataTableのProfileID行の条件で没収し、行の服を着せる。何か没収した、または服を着せたらtrue。
	 *  すでに同じ保管先に没収品がある場合は、そこへ追加する（装備の記録は最初の没収時のものを残す） */
	UFUNCTION(BlueprintCallable, Category = "Confiscation")
	bool ConfiscateByProfile(FName ProfileID);

	/** 保管先StashIDの没収品を返却する。カバンに入り切らない分は保管されたまま残る。
	 *  保管先が無い場合はfalse。返却を試みたらtrue（残りがあってもtrue） */
	UFUNCTION(BlueprintCallable, Category = "Confiscation")
	bool ReturnStash(FName StashID);

	/** 保管中のすべての没収品を返却する */
	UFUNCTION(BlueprintCallable, Category = "Confiscation")
	void ReturnAllStashes();

	/** 保管先StashIDに没収品があるか */
	UFUNCTION(BlueprintPure, Category = "Confiscation")
	bool HasStash(FName StashID) const { return Stashes.Contains(StashID); }
};
