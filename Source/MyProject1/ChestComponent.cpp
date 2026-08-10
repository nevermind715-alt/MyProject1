#include "ChestComponent.h"
#include "Engine/DataTable.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"
#include "MyProject1HUD.h"
#include "GameFramework/PlayerController.h"

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
			// 既に入っている数と要求された数を比べ、少ない方を実際に渡すことにする
			int32 AmountToGive = FMath::Min(ChestContents[i].Quantity, RequestAmount);

			// 無限モードの場合は、要求された数をそのまま渡す
			if (bIsInfinite)
			{
				AmountToGive = RequestAmount;
			}

			// プレイヤーのインベントリに追加
			bool bAdded = InteractingPlayer->InventoryComp->AddItem(ItemID, AmountToGive);

			if (bAdded)
			{
				// 無限モードでなければ在庫を減らす
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

				// 使い切り設定かつ中身が空になった場合、チェスト（親Actor）を消す
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
				// カバンが満杯で入らなかった場合など
				return false;
			}
		}
	}

	return false; // 該当アイテムが見つからなかった
}

void UChestComponent::Interact(AMyProject1Character* InteractingPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("[Chest] Interact() called. bIsOpen=%s"), bIsOpen ? TEXT("true") : TEXT("false"));

	if (!InteractingPlayer) return;

	// ★排他制御：インベントリのアクションメニューが開いたままチェストを開けると
	// メニューが消えなくなる不具合を防ぐため、開いていれば強制的に閉じさせる
	if (InteractingPlayer->InventoryComp && InteractingPlayer->InventoryComp->bIsItemActionMenuOpen)
	{
		InteractingPlayer->InventoryComp->SetItemActionMenuState(false);
	}

	// ★世界に1つしかない（bIsRare）アイテムを既にプレイヤーが入手済みなら、
	// チェストの中身から取り除いておく（レベル移動でチェストが初期状態に戻っても、
	// 見た目上「取れないアイテム」が残らないようにするため）
	if (InteractingPlayer->InventoryComp)
	{
		UInventoryComponent* Inv = InteractingPlayer->InventoryComp;
		for (int32 i = ChestContents.Num() - 1; i >= 0; --i)
		{
			FItemData ItemInfo;
			if (Inv->GetItemDataBP(ChestContents[i].ItemID, ItemInfo) &&
				ItemInfo.bIsRare && Inv->ObtainedRareItemIDs.Contains(ChestContents[i].ItemID))
			{
				ChestContents.RemoveAt(i);
			}
		}
	}

	// 初回のインタラクトの時だけ、ふたを開くアニメーションを再生する
	if (!bIsOpen)
	{
		bIsOpen = true;

		UE_LOG(LogTemp, Warning, TEXT("[Chest] Broadcasting OnChestOpenRequested"));

		// 実際にどう回転させるかはBP_Chest側のTimelineに任せる。
		// C++は「開いた」という状態変化と、再生の合図だけを出す。
		OnChestOpenRequested.Broadcast();
	}

	// アイテム一覧HUD（既存のWBP_ChestMenu）を表示する
	APlayerController* PC = Cast<APlayerController>(InteractingPlayer->GetController());
	if (!PC) return;

	AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD());
	if (!HUD) return;

	HUD->CurrentChestComp = this;
	HUD->ToggleChestMenu();
}

void UChestComponent::CloseChest()
{
	UE_LOG(LogTemp, Warning, TEXT("[Chest] CloseChest() called. bIsOpen=%s"), bIsOpen ? TEXT("true") : TEXT("false"));

	// 開いていない蓋を閉めようとしても何もしない（多重呼び出し防止）
	if (!bIsOpen) return;

	bIsOpen = false;

	UE_LOG(LogTemp, Warning, TEXT("[Chest] Broadcasting OnChestCloseRequested"));

	// Interact()の開く処理と対になる、閉じる合図
	OnChestCloseRequested.Broadcast();
}
