#include "WBP_EquipItemList.h"
#include "WBP_EquipItemRow.h"
#include "WBP_EquipDetail.h"
#include "WBP_EquipmentMenu.h"
#include "InventoryComponent.h"
#include "MyProject1Character.h"
#include "MyProject1HUD.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UWBP_EquipItemList::NativeConstruct()
{
	Super::NativeConstruct();

	// 診断: BindWidgetで結び付かなかった場合に備え、名前でも探す。ウィジェットツリー内の名前も出す。
	if (!ScrollBox_Items)
	{
		ScrollBox_Items = Cast<UScrollBox>(GetWidgetFromName(TEXT("ScrollBox_Items")));
		UE_LOG(LogTemp, Warning, TEXT("[EquipList] ScrollBox_Items がBindされていなかった。名前検索の結果=%s"), ScrollBox_Items ? TEXT("見つかった") : TEXT("見つからない"));
		if (WidgetTree)
		{
			WidgetTree->ForEachWidget([](UWidget* W)
			{
				UE_LOG(LogTemp, Warning, TEXT("[EquipList]   ツリー内ウィジェット: %s (%s)"), *W->GetName(), *W->GetClass()->GetName());
			});
		}
	}

	if (Btn_Close)
	{
		Btn_Close->OnClicked.RemoveDynamic(this, &UWBP_EquipItemList::HandleCloseClicked);
		Btn_Close->OnClicked.AddDynamic(this, &UWBP_EquipItemList::HandleCloseClicked);
	}
	if (Btn_Unequip)
	{
		Btn_Unequip->OnClicked.RemoveDynamic(this, &UWBP_EquipItemList::HandleUnequipClicked);
		Btn_Unequip->OnClicked.AddDynamic(this, &UWBP_EquipItemList::HandleUnequipClicked);
	}

	// 何も選んでいない間は詳細を隠す
	if (WBP_EquipDetail)
	{
		WBP_EquipDetail->UpdateEquipDetail(NAME_None, false);
	}

	UpdateList();
	UE_LOG(LogTemp, Log, TEXT("[EquipList] 構築: スロット=%d, ScrollBox=%s, 行クラス=%s, 子の数=%d"), static_cast<int32>(TargetSlot),
		ScrollBox_Items ? TEXT("bound") : TEXT("null"), *GetNameSafe(RowWidgetClass), ScrollBox_Items ? ScrollBox_Items->GetChildrenCount() : -1);
}

void UWBP_EquipItemList::UpdateList()
{
	if (!ScrollBox_Items) return;

	ScrollBox_Items->ClearChildren();

	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	if (!RowWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipList] RowWidgetClass が未設定。WBP_EquipItemList のクラスデフォルトで WBP_EquipItemRow を指定してください。"));
		return;
	}
	if (RowWidgetClass && RowWidgetClass->IsNative())
	{
		UE_LOG(LogTemp, Error, TEXT("[EquipList] RowWidgetClass にC++クラス自体が指定されている。Blueprintアセット(WBP_EquipItemRow)を選び直してください。"));
		return;
	}
	if (!Player || !Player->InventoryComp || !Player->EquipmentDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipList] 一覧を作れない: Player=%s, InventoryComp=%s, EquipmentDataTable=%s"),
			*GetNameSafe(Player), Player ? *GetNameSafe(Player->InventoryComp) : TEXT("-"), Player ? *GetNameSafe(Player->EquipmentDataTable) : TEXT("-"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[EquipList] 一覧作成: スロット=%d, カバンのスロット数=%d, 行クラス=%s"),
		static_cast<int32>(TargetSlot), Player->InventoryComp->InventoryContent.Num(), *GetNameSafe(RowWidgetClass));

	APlayerController* PC = GetOwningPlayer();
	const FName CurrentlyEquipped = Player->GetEquippedItemID(TargetSlot);

	for (const FInventorySlot& Entry : Player->InventoryComp->InventoryContent)
	{
		const FEquipmentData* EquipData = Player->EquipmentDataTable->FindRow<FEquipmentData>(Entry.ItemID, TEXT("EquipItemList"));
		if (!EquipData)
		{
			UE_LOG(LogTemp, Log, TEXT("[EquipList]   %s: DT_Equipmentsに行なし（装備品ではない）"), *Entry.ItemID.ToString());
			continue;
		}
		if (EquipData->TargetSlot != TargetSlot)
		{
			UE_LOG(LogTemp, Log, TEXT("[EquipList]   %s: 部位が違う(%d)"), *Entry.ItemID.ToString(), static_cast<int32>(EquipData->TargetSlot));
			continue;
		}
		UE_LOG(LogTemp, Log, TEXT("[EquipList]   %s: 一覧に追加"), *Entry.ItemID.ToString());

		UWBP_EquipItemRow* Row = CreateWidget<UWBP_EquipItemRow>(PC, RowWidgetClass);
		if (!Row) continue;

		Row->SetupRow(Entry.ItemID, this, WBP_EquipDetail, Entry.ItemID == CurrentlyEquipped);
		ScrollBox_Items->AddChild(Row);
	}
}

void UWBP_EquipItemList::OnRowSelected(FName ItemID)
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	if (!Player) return;

	// 自力で外せない装備が着いている部位などで失敗した場合は、一覧を開いたままにする
	if (!Player->EquipItemFromInventory(ItemID)) return;

	if (EquipSound)
	{
		UGameplayStatics::PlaySound2D(this, EquipSound);
	}

	if (ParentMenu)
	{
		ParentMenu->UpdateEquipmentDisplay();
	}
	CloseList();
}

void UWBP_EquipItemList::HandleUnequipClicked()
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	if (!Player) return;

	Player->TryUnequipItem(TargetSlot);

	if (ParentMenu)
	{
		ParentMenu->UpdateEquipmentDisplay();
	}
	if (UnequipSound)
	{
		UGameplayStatics::PlaySound2D(this, UnequipSound);
	}

	// 「装備中」表示を更新する
	UpdateList();
	UE_LOG(LogTemp, Log, TEXT("[EquipList] 構築: スロット=%d, ScrollBox=%s, 行クラス=%s, 子の数=%d"), static_cast<int32>(TargetSlot),
		ScrollBox_Items ? TEXT("bound") : TEXT("null"), *GetNameSafe(RowWidgetClass), ScrollBox_Items ? ScrollBox_Items->GetChildrenCount() : -1);
}

void UWBP_EquipItemList::HandleCloseClicked()
{
	CloseList();
}

void UWBP_EquipItemList::CloseList()
{
	RemoveFromParent();

	APlayerController* PC = GetOwningPlayer();
	if (AMyProject1HUD* HUD = PC ? Cast<AMyProject1HUD>(PC->GetHUD()) : nullptr)
	{
		// 一覧が閉じたので、記憶していたサブメニューを空に戻す（装備メニュー自体は ToggleEquipmentMenu 側が閉じる）。
		// ここで装備メニューを入れてしまうと、次のスペースキーで ToggleEquipmentMenu を通らずに消え、コマンドメニューが戻らなくなる。
		HUD->ActiveSubMenuWidget = nullptr;
	}
}