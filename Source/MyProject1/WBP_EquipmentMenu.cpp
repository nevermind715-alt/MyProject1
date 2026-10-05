#include "WBP_EquipmentMenu.h"
#include "WBP_EquipItemList.h"
#include "InventoryComponent.h"
#include "MyProject1Character.h"
#include "MyProject1HUD.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"

UTextBlock* UWBP_EquipmentMenu::GetSlotText(EEquipmentSlot EquipSlot) const
{
	switch (EquipSlot)
	{
		case EEquipmentSlot::Weapon: return Txt_Wepon;
		case EEquipmentSlot::Head: return Txt_Head;
		case EEquipmentSlot::Neck: return Txt_Neck;
		case EEquipmentSlot::Torso: return Txt_Torso;
		case EEquipmentSlot::Waist: return Txt_Waist;
		case EEquipmentSlot::Wrist: return Txt_Wrist;
		case EEquipmentSlot::Hands: return Txt_Hand;
		case EEquipmentSlot::Legs: return Txt_Legs;
		case EEquipmentSlot::Ankle: return Txt_Ankle;
		case EEquipmentSlot::Feet: return Txt_Foot;
		case EEquipmentSlot::InnerUpper: return Txt_InnerUpper;
		case EEquipmentSlot::InnerLower: return Txt_InnerLower;
		case EEquipmentSlot::Extra1: return Txt_Ather1;
		case EEquipmentSlot::Extra2: return Txt_Ather2;
		case EEquipmentSlot::Extra3: return Txt_Ather3;
		case EEquipmentSlot::Extra4: return Txt_Ather4;
		case EEquipmentSlot::Extra5: return Txt_Ather5;
		default: return nullptr;
	}
}

void UWBP_EquipmentMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 各部位ボタンを1回だけバインドする
	UE_LOG(LogTemp, Log, TEXT("[EquipMenu] 初期化: EquipItemListClass=%s, Btn_Torso=%s, Txt_Torso=%s"), *GetNameSafe(EquipItemListClass), Btn_Torso ? TEXT("bound") : TEXT("null"), Txt_Torso ? TEXT("bound") : TEXT("null"));
	if (Btn_Wepon) Btn_Wepon->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleWeponClicked);
	if (Btn_Head) Btn_Head->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleHeadClicked);
	if (Btn_Neck) Btn_Neck->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleNeckClicked);
	if (Btn_Torso) Btn_Torso->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleTorsoClicked);
	if (Btn_Waist) Btn_Waist->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleWaistClicked);
	if (Btn_Wrist) Btn_Wrist->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleWristClicked);
	if (Btn_Hand) Btn_Hand->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleHandClicked);
	if (Btn_Legs) Btn_Legs->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleLegsClicked);
	if (Btn_Ankle) Btn_Ankle->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleAnkleClicked);
	if (Btn_Foot) Btn_Foot->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleFootClicked);
	if (Btn_InnerU) Btn_InnerU->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleInnerUClicked);
	if (Btn_InnerL) Btn_InnerL->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleInnerLClicked);
	if (Btn_Ather1) Btn_Ather1->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleAther1Clicked);
	if (Btn_Ather2) Btn_Ather2->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleAther2Clicked);
	if (Btn_Ather3) Btn_Ather3->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleAther3Clicked);
	if (Btn_Ather4) Btn_Ather4->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleAther4Clicked);
	if (Btn_Ather5) Btn_Ather5->OnClicked.AddDynamic(this, &UWBP_EquipmentMenu::HandleAther5Clicked);
}

void UWBP_EquipmentMenu::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateEquipmentDisplay();
}

void UWBP_EquipmentMenu::UpdateEquipmentDisplay()
{
	const AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwningPlayerPawn());
	if (!Player) return;

	// 表示する部位（髪型はこのメニューに無い）
	static const EEquipmentSlot DisplaySlots[] = {
		EEquipmentSlot::Weapon, EEquipmentSlot::Head, EEquipmentSlot::Neck, EEquipmentSlot::Torso,
		EEquipmentSlot::Waist, EEquipmentSlot::Wrist, EEquipmentSlot::Hands, EEquipmentSlot::Legs,
		EEquipmentSlot::Ankle, EEquipmentSlot::Feet, EEquipmentSlot::InnerUpper, EEquipmentSlot::InnerLower,
		EEquipmentSlot::Extra1, EEquipmentSlot::Extra2, EEquipmentSlot::Extra3, EEquipmentSlot::Extra4, EEquipmentSlot::Extra5 };

	for (const EEquipmentSlot EquipSlot : DisplaySlots)
	{
		UTextBlock* Text = GetSlotText(EquipSlot);
		if (!Text) continue;

		FString DisplayName = TEXT("---");
		if (const FName* EquippedID = Player->CurrentEquippedItems.Find(EquipSlot))
		{
			if (!EquippedID->IsNone())
			{
				DisplayName = EquippedID->ToString();

				FItemData Data;
				if (Player->InventoryComp && Player->InventoryComp->GetItemDataBP(*EquippedID, Data))
				{
					DisplayName = Data.Name;
				}
				else if (Player->EquipmentDataTable)
				{
					if (const FEquipmentData* EquipData = Player->EquipmentDataTable->FindRow<FEquipmentData>(*EquippedID, TEXT("EquipmentMenuDisplay")))
					{
						if (!EquipData->DevMemo.IsEmpty()) DisplayName = EquipData->DevMemo;
					}
				}
			}
		}
		Text->SetText(FText::FromString(DisplayName));
	}
}

void UWBP_EquipmentMenu::OpenEquipList(EEquipmentSlot TargetEquipSlot)
{
	UE_LOG(LogTemp, Log, TEXT("[EquipMenu] OpenEquipList 呼ばれた: スロット=%d"), static_cast<int32>(TargetEquipSlot));
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	if (!EquipItemListClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipMenu] EquipItemListClass が未設定。WBP_EquipmentMenu のクラスデフォルトで WBP_EquipItemList を指定してください。"));
		return;
	}

	if (EquipItemListClass && EquipItemListClass->IsNative())
	{
		UE_LOG(LogTemp, Error, TEXT("[EquipMenu] EquipItemListClass にC++クラス自体が指定されている。Blueprintアセット(WBP_EquipItemList)を選び直してください。"));
		return;
	}
	if (CurrentListWidget)
	{
		CurrentListWidget->RemoveFromParent();
		CurrentListWidget = nullptr;
	}

	UWBP_EquipItemList* List = CreateWidget<UWBP_EquipItemList>(PC, EquipItemListClass);
	if (!List) return;

	List->TargetSlot = TargetEquipSlot;
	List->ParentMenu = this;
	List->AddToViewport(30);
	CurrentListWidget = List;
	UE_LOG(LogTemp, Log, TEXT("[EquipMenu] 装備一覧を生成して画面に追加した: %s (InViewport=%d)"), *GetNameSafe(List), List->IsInViewport());

	// Escキー等で閉じる対象を、開いた一覧にする
	if (AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD()))
	{
		HUD->ActiveSubMenuWidget = List;
	}
}

void UWBP_EquipmentMenu::HandleWeponClicked() { OpenEquipList(EEquipmentSlot::Weapon); }
void UWBP_EquipmentMenu::HandleHeadClicked() { OpenEquipList(EEquipmentSlot::Head); }
void UWBP_EquipmentMenu::HandleNeckClicked() { OpenEquipList(EEquipmentSlot::Neck); }
void UWBP_EquipmentMenu::HandleTorsoClicked() { OpenEquipList(EEquipmentSlot::Torso); }
void UWBP_EquipmentMenu::HandleWaistClicked() { OpenEquipList(EEquipmentSlot::Waist); }
void UWBP_EquipmentMenu::HandleWristClicked() { OpenEquipList(EEquipmentSlot::Wrist); }
void UWBP_EquipmentMenu::HandleHandClicked() { OpenEquipList(EEquipmentSlot::Hands); }
void UWBP_EquipmentMenu::HandleLegsClicked() { OpenEquipList(EEquipmentSlot::Legs); }
void UWBP_EquipmentMenu::HandleAnkleClicked() { OpenEquipList(EEquipmentSlot::Ankle); }
void UWBP_EquipmentMenu::HandleFootClicked() { OpenEquipList(EEquipmentSlot::Feet); }
void UWBP_EquipmentMenu::HandleInnerUClicked() { OpenEquipList(EEquipmentSlot::InnerUpper); }
void UWBP_EquipmentMenu::HandleInnerLClicked() { OpenEquipList(EEquipmentSlot::InnerLower); }
void UWBP_EquipmentMenu::HandleAther1Clicked() { OpenEquipList(EEquipmentSlot::Extra1); }
void UWBP_EquipmentMenu::HandleAther2Clicked() { OpenEquipList(EEquipmentSlot::Extra2); }
void UWBP_EquipmentMenu::HandleAther3Clicked() { OpenEquipList(EEquipmentSlot::Extra3); }
void UWBP_EquipmentMenu::HandleAther4Clicked() { OpenEquipList(EEquipmentSlot::Extra4); }
void UWBP_EquipmentMenu::HandleAther5Clicked() { OpenEquipList(EEquipmentSlot::Extra5); }
