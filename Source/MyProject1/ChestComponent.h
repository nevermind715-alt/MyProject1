#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h"
#include "ChestComponent.generated.h"

// UI側で「チェストの中身が変化した」ことを検知するためのデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestUpdated);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UChestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UChestComponent();

	// 中身が変化した時にUIに知らせる合図
	UPROPERTY(BlueprintAssignable, Category = "Chest|UI")
	FOnChestUpdated OnChestUpdated;

protected:
	virtual void BeginPlay() override;

public:
	// ==========================================
	// 設定項目 (エディタのプロパティで設定)
	// ==========================================

	/** Trueならデータテーブルから全アイテムを自動生成して格納する（デバッグ用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bGenerateAllItems = false;

	/** デバッグ自動生成時に、各アイテムを何個ずつ箱に入れるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings", meta = (EditCondition = "bGenerateAllItems"))
	int32 DebugItemQuantity = 99;

	/** Trueなら何度取り出しても中身が減らない（無限箱） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bIsInfinite = false;

	/** Trueなら中身が空になった時に親Actor（チェスト自体）をワールドから削除する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings")
	bool bDestroyWhenEmpty = false;

	/** 読み込むアイテムリストのデータテーブル（DT_Itemsなどをセットする） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Settings", meta = (EditCondition = "bGenerateAllItems"))
	UDataTable* ItemDataTable;

	// ==========================================
	// データ (現在の中身)
	// ==========================================

	/** チェストの中身。bGenerateAllItemsがFalseの場合は、手動でアイテムを設定します。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Content")
	TArray<FInventorySlot> ChestContents;

	// ==========================================
	// 機能
	// ==========================================

	/** プレイヤーがチェストから特定のアイテムを取り出す処理 */
	UFUNCTION(BlueprintCallable, Category = "Chest")
	bool TakeItem(FName ItemID, int32 RequestAmount, class AMyProject1Character* InteractingPlayer);

	/** 現在のチェストの中身を取得する（UI表示用） */
	UFUNCTION(BlueprintPure, Category = "Chest")
	TArray<FInventorySlot> GetChestContents() const { return ChestContents; }
};