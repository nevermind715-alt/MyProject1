#pragma once

#include "CoreMinimal.h"
#include "MyProject1Character.h"
#include "MyProject1Types.h"
#include "QuestNPCBase.generated.h"

// クエストを提示するNPCの共通C++基底クラス。AMyProject1Characterを継承することで、
// 移動（AMyAIController）・アニメーション（UMyProject1AnimInstanceBase）・装備（Hair/Face含む全メッシュスロット）の
// 仕組みをそのまま流用できる（これらはコンポーネント化されておらずAMyProject1Character本体に組み込まれているため、
// 継承せずに一部だけ切り出すと結局同じ仕組みを複製することになり非効率）。
// 戦闘機能自体は使わない想定だが、bIsActiveEnemy=falseにしておけばAMyAIControllerが攻撃状態に移行しないため、
// 実害はない。
// 複数のFQuestDialogSetを優先順位付きで持てるので、1体のNPCが「クエスト1完了後は自動でクエスト2を提示する」
// といった連鎖・分岐を、Blueprint側に判定ロジックを書かずに表現できる。
UCLASS()
class MYPROJECT1_API AQuestNPCBase : public AMyProject1Character
{
	GENERATED_BODY()

public:
	AQuestNPCBase();

protected:
	virtual void BeginPlay() override;

public:
	// ログ表示等に使うNPCの名前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	FString NPCName;

	// このNPCが使う会話データテーブル（DT_Dialogs）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	class UDataTable* DialogTable;

	// このNPCが扱うクエストのリスト。配列の先頭ほど優先度が高い。
	// 進行中/報告待ちのクエストがあればそれを継続表示し、無ければ受注条件を満たした先頭の未受注クエストを提示する
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC")
	TArray<FQuestDialogSet> Quests;

	// --- 初回だけの挨拶（クエストと無関係な一度きりの状態変化用） ---
	// 空欄なら使わない（従来通りQuests配列のみで判定）。設定すると、このフラグを未取得の間だけ
	// DialogRowName_FirstMeetを優先表示する。フラグの付与はDT_Dialogs側の対象行のGrantFlagで行う想定
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|FirstMeet")
	FName FirstMeetFlag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|FirstMeet")
	FName DialogRowName_FirstMeet;

	// BPI_InteractableのイベントからEキー等で呼び出す、会話開始の共通ネイティブ処理
	UFUNCTION(BlueprintCallable, Category = "QuestNPC")
	void TalkToNPC(class AMyProject1Character* PlayerChar);

	// --- フラグによる表示/非表示切り替え ---
	// 例：「元の場所のNPC」にHideOnFlag、「移動先に先置きした同じNPC」にShowOnFlagを
	// 同じフラグ名で設定しておくと、会話でフラグが立った瞬間に入れ替わったように見える
	/** 表示切り替えの判定に使うフラグ（空欄、またはVisibilityModeがNoneなら常に表示のまま） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Flag Visibility")
	FName VisibilityFlag;

	/** VisibilityFlagの状態に応じて、表示するか消えるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Flag Visibility")
	EFlagVisibilityMode VisibilityMode = EFlagVisibilityMode::None;

	// --- NPC装備（見た目のみ） ---
	// DT_Equipments（EquipmentDataTable、AMyProject1Character由来）のRowNameを並べるだけで、
	// BeginPlay時に継承済みのEquipItem()経由でスケルタルメッシュ／スタティックメッシュ／スキンオーバーレイの
	// 全てを反映できる。個別メッシュを直接指定する必要がなくなり、DataTable側の編集だけで見た目を差し替えられる。
	// StatModifiers（ステータス補正）はShouldApplyEquipmentStatBonuses()のオーバーライドにより無視されるため、
	// ここに指定した装備はMyStatsに一切影響しない（純粋に見た目だけ）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Equipment")
	TArray<FName> InitialEquipmentRowNames;

	// DT_Equipmentsと同じ考え方で、SkinOverlayComp側の刺青・傷跡もRow名を並べるだけで初期化できるようにする
	// （SkinOverlayComp->TattooDataTable / ScarDataTableのRowName。テーブル自体はBP DetailsのSkinOverlayCompで設定する）。
	// AddOverlay()経由で反映されるが、これもShouldApplyEquipmentStatBonuses()により見た目だけでMyStatsは変化しない
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Equipment")
	TArray<FName> InitialTattooOverlayRowNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QuestNPC|Equipment")
	TArray<FName> InitialScarOverlayRowNames;

protected:
	// 装備によるステータス補正（StatModifiers）を無効化し、NPCの装備を見た目だけの変化にする
	virtual bool ShouldApplyEquipmentStatBonuses() const override { return false; }

private:
	/** InitialEquipmentRowNamesをEquipmentDataTableから引いて、BeginPlay時に見た目だけ反映する */
	void ApplyInitialEquipment();

	/** VisibilityFlag/VisibilityModeの設定に応じて、現在の表示/当たり判定を更新する */
	void UpdateFlagVisibility(const class AMyProject1Character* PlayerChar);

	/** bHasFlag（フラグを持っているか）を基に表示/当たり判定を反映する共通処理
	    （UpdateFlagVisibility・OnPlayerFlagAdded・OnPlayerFlagRemovedの実体を1箇所にまとめたもの） */
	void ApplyVisibilityForFlagState(bool bHasFlag);

	/** プレイヤーがフラグを獲得した時に呼ばれる（OnFlagAddedの購読先）。VisibilityFlag一致時のみ表示を更新する */
	UFUNCTION()
	void OnPlayerFlagAdded(FName FlagName);

	/** プレイヤーがフラグを失った時に呼ばれる（OnFlagRemovedの購読先）。VisibilityFlag一致時のみ表示を更新する。
	    これが無いとRemoveFlagで消したフラグにNPCの表示/非表示が追従せず、部屋の出入りを挟んでも
	    見た目（表示状態）が古いままになる */
	UFUNCTION()
	void OnPlayerFlagRemoved(FName FlagName);
};
