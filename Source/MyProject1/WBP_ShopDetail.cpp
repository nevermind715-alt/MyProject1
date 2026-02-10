#include "WBP_ShopDetail.h"
#include "MyProject1Character.h"
#include "InventoryComponent.h"

void UWBP_ShopDetail::HandleHoverChanged(FName NewItemID)
{
	AMyProject1Character* PC = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	UInventoryComponent* Inv = PC ? PC->FindComponentByClass<UInventoryComponent>() : nullptr;

	if (!Inv) 
	{		
		OnUpdateDetailDisplay(FItemData(), false);
		return;
	}

	FItemData Data;
	if (!NewItemID.IsNone() && Inv && Inv->GetItemDataBP(NewItemID, Data))
	{
		OnUpdateDetailDisplay(Data, true);
	}
	else
	{		
		OnUpdateDetailDisplay(FItemData(), false);
	}
}

void UWBP_ShopDetail::NativeConstruct()
{
	Super::NativeConstruct(); // 親クラス（UserWidget）の初期化を必ず呼ぶ

	if (AMyProject1Character* PC = Cast<AMyProject1Character>(GetOwningPlayerPawn()))
	{
		if (UInventoryComponent* Inv = PC->FindComponentByClass<UInventoryComponent>())
		{
			// 「アイテムホバーが変わったら、私の HandleHoverChanged を実行して！」と予約する
			Inv->OnItemHoverChanged.AddDynamic(this, &UWBP_ShopDetail::HandleHoverChanged);
		}
	}

	
}