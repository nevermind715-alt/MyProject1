#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h"
#include "ChestComponent.generated.h"

// UIに「チェストの中身が変化した」ことを知らせるためのデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestUpdated);

// BP_Chest側に「今から蓋を開閉する」ことを知らせるためのデリゲート（実際の見た目はBP側のTimelineが担当）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestOpenRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestCloseRequested);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UChestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChestComponent();

	// 中身が変化した時にUIへ知らせる合図
	UPROPERTY(BlueprintAssignable, Category = "Chest|UI")
	FOnChestUpdated OnChestUpdated;

protected:
	virtual void BeginPlay() override;

public:
	// ==========================================
	// 表示名（WBP_NPCNameに渡す用）
	// ==========================================

	/** ボックスコリジョンにオーバーラップした時、WBP_NPCNameに表示する名前 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Display")
	FString ChestName = TEXT("宝箱");

	// ==========================================
	// 設定項目（エディタのプロパティで設定）
	// ==========================================

	/** Trueならデータテーブルから全アイテムを自動生成して格納する（デバッグ用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bGenerateAllItems = false;

	/** デバッグ自動生成時に、各アイテムを何個ずつ入れるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings", meta = (EditCondition = "bGenerateAllItems"))
	int32 DebugItemQuantity = 99;

	/** Trueなら何度取り出しても中身が減らない（無限箱） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bIsInfinite = false;

	/** Trueなら中身が空になった時に親Actor（チェスト本体）をワールドから削除する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bDestroyWhenEmpty = false;

	/** 読み込むアイテムリストのデータテーブル（DT_Itemsなどをセットする） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings", meta = (EditCondition = "bGenerateAllItems"))
	UDataTable* ItemDataTable;

	// ==========================================
	// データ（現在の中身）
	// ==========================================

	/** チェストの中身。bGenerateAllItemsがFalseの場合は、手動でアイテムを設定します。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Content")
	TArray<FInventorySlot> ChestContents;

	// ==========================================
	// 機能
	// ==========================================

	/** プレイヤーがチェストから指定のアイテムを取り出す */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	bool TakeItem(FName ItemID, int32 RequestAmount, class AMyProject1Character* InteractingPlayer);

	/** 現在のチェストの中身を取得する（UI表示用） */
	UFUNCTION(BlueprintPure, Category = "Chest")
	TArray<FInventorySlot> GetChestContents() const { return ChestContents; }

	// ==========================================
	// 開閉（蓋）状態＋Eキーインタラクト
	// ==========================================

	/** 現在開いているか（二重アニメーション防止と実インタラクト判定の両方に使う） */
	UPROPERTY(BlueprintReadOnly, Category = "Chest|State")
	bool bIsOpen = false;

	/** BPI_InteractableのEキーイベントから呼ばれる、蓋の開閉処理（開く＋HUD表示の切替） */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	void Interact(class AMyProject1Character* InteractingPlayer);

	/** BPI_TargetableのbIsTargeted=false（カーソルが外れた）時に呼ばれる、閉じる処理 */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	void CloseChest();

	// ==========================================
	// 蓋の見た目（実際の回転演出）はBP_Chest側のTimelineに任せる。
	// C++は「いつ再生するか」だけをデリゲートで合図として送る。
	// ==========================================

	/** 初回インタラクト時にブロードキャストされる。BP_Chest側でこれをバインドし、Timelineを順再生する */
	UPROPERTY(BlueprintAssignable, Category = "Chest|Animation")
	FOnChestOpenRequested OnChestOpenRequested;

	/** ターゲットカーソルが消えた時にブロードキャストされる。BP_Chest側でこれをバインドし、Timelineを逆再生する */
	UPROPERTY(BlueprintAssignable, Category = "Chest|Animation")
	FOnChestCloseRequested OnChestCloseRequested;
};
