// InventoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h"
#include "InventoryComponent.generated.h"

// ★デリゲートの宣言（ItemIDを渡すために OneParam を使用）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemHoverChanged, FName, ItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	// UIがこれを購読（Bind）する
	UPROPERTY(BlueprintAssignable, Category = "Inventory|UI")
	FOnItemHoverChanged OnItemHoverChanged;

	// スロットがマウスホバー時に呼ぶ関数
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void ReportItemHover(FName ItemID) { OnItemHoverChanged.Broadcast(ItemID); }

protected:
	virtual void BeginPlay() override;

public:
	// --- 設定項目 ---

	// カバンの最大容量（FF11初期なら30、最大80とか）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 MaxSlots = 30;

	// アイテムデータが書かれているデータテーブル（エディタでセットする）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	UDataTable* ItemDataTable;

	// 所持金（ギル）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Gil = 0;

	/** アイテムの購入処理（代金の支払いとログ出力も行う） */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ProcessPurchase(FName ItemID, int32 Amount);

	/** アイテムの売却処理（アイテム削除、代金受取、ログ出力） */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ProcessSale(FName ItemID, int32 Amount = 1);

	// --- カバンの実体 ---

	// 実際にアイテムが入っているリスト
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> InventoryContent;

	// UI更新用のイベントディスパッチャー
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

public:
	// --- 機能関数 ---

	/** アイテムを追加する（拾う/買う） */
	// 戻り値: 全部拾えたら true、カバンがいっぱいで拾えなかったら false
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemID, int32 Amount);

	/** アイテムを減らす（使う/捨てる） */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Amount);

	/** 特定のアイテムを何個持っているか確認する */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemQuantity(FName ItemID);

	/** 所持金を追加/消費する */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddGil(int32 Amount);

	/** お金が足りているかチェックして、足りていれば支払う */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TrySpendGil(int32 Amount);

	// アイテムを使う関数
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(FName ItemID);

	// Blueprintから呼び出せるように公開
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemDataBP(FName ItemID, FItemData& OutData);

	
protected:
	/** データテーブルからアイテム情報を取得する便利関数（内部ポインタ用） */
	FItemData* GetItemData(FName ItemID);

};