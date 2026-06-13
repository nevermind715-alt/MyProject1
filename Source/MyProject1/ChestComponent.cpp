#include "ChestComponent.h"
#include "Engine/DataTable.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"

// コンストラクタ
UChestComponent::UChestComponent()
{
	// 毎フレームの処理は不要なのでOFFにして軽量化
	PrimaryComponentTick.bCanEverTick = false;
}

void UChestComponent::BeginPlay()
{
	Super::BeginPlay();

	// デバッグ用：データテーブルから全アイテムを自動生成して箱に入れる
	if (bGenerateAllItems && ItemDataTable)
	{
		// 既存の中身を一旦クリア
		ChestContents.Empty();

		// データテーブルの全行名（ItemID）を取得
		TArray<FName> RowNames = ItemDataTable->GetRowNames();

		for (const FName& RowName : RowNames)
		{
			FInventorySlot NewSlot;
			NewSlot.ItemID = RowName;
			NewSlot.Quantity = DebugItemQuantity;
			ChestContents.Add(NewSlot);
		}
	}
}

bool UChestComponent::TakeItem(FName ItemID, int32 RequestAmount, AMyProject1Character* InteractingPlayer)
{
	if (RequestAmount <= 0 || !InteractingPlayer || !InteractingPlayer->InventoryComp)
	{
		return false;
	}

	// 箱の中身から指定されたアイテムを探す
	for (int32 i = 0; i < ChestContents.Num(); ++i)
	{
		if (ChestContents[i].ItemID == ItemID)
		{
			// 箱に入っている数と要求された数を比べ、少ない方を実際に渡す数とする
			int32 AmountToGive = FMath::Min(ChestContents[i].Quantity, RequestAmount);

			// 無限箱の場合は、要求された数をそのまま渡す
			if (bIsInfinite)
			{
				AmountToGive = RequestAmount;
			}

			// プレイヤーのインベントリに追加
			bool bAdded = InteractingPlayer->InventoryComp->AddItem(ItemID, AmountToGive);

			if (bAdded)
			{
				// 無限箱でなければ在庫を減らす
				if (!bIsInfinite)
				{
					ChestContents[i].Quantity -= AmountToGive;

					// 在庫が0になったらリストから削除
					if (ChestContents[i].Quantity <= 0)
					{
						ChestContents.RemoveAt(i);
					}
				}

				// UI側に中身が更新されたことを通知
				OnChestUpdated.Broadcast();

				// 使い切り設定かつ中身が空っぽになった場合、チェスト（親Actor）を消去
				if (bDestroyWhenEmpty && ChestContents.Num() == 0)
				{
					if (AActor* OwnerActor = GetOwner())
					{
						OwnerActor->Destroy();
					}
				}

				return true; // 取り出し成功
			}
			else
			{
				// カバンがいっぱいで入らなかった場合など
				return false;
			}
		}
	}

	return false; // 該当アイテムが見つからなかった
}