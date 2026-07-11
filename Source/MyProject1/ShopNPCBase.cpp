#include "ShopNPCBase.h"
#include "MyProject1Character.h"
#include "MyProject1HUD.h"
#include "GameFramework/PlayerController.h"

AShopNPCBase::AShopNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(FName("NPC"));
	ShopLevel = 1;
	// 初期値を列挙型の安全な形式に変更
	ShopModeCategory = EShopModeCategory::Tattoo;
	NPCName = TEXT("ショップ店員");
	GreetingText = TEXT("へい、いらっしゃい！");
	GoodbyeText = TEXT("またいつでも来な。");
}

void AShopNPCBase::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(FName("NPC"));
}

// 新設：列挙型をデータテーブル検索用の正しい「FName（文字）」へ内部変換してBlueprintに返す関数
FName AShopNPCBase::GetShopModeCategoryAsName() const
{
	switch (ShopModeCategory)
	{
	case EShopModeCategory::Tattoo:   return FName("Tattoo");
	case EShopModeCategory::Scar:     return FName("Scar");
	case EShopModeCategory::Piercing: return FName("Piercing");
	case EShopModeCategory::Disease:  return FName("Disease");
	default:                          return FName("Tattoo");
	}
}

TArray<FName> AShopNPCBase::GetAvailableShopItems() const
{
	TArray<FName> AvailableItems;

	// データテーブルがセットされていない場合は空の配列を返す
	if (!ItemDataTable) return AvailableItems;

	// 自身のショップカテゴリに対応するアイテムタイプをマッピング
	EItemType TargetType;
	bool bIsNormalShop = false;

	switch (ShopModeCategory)
	{
	case EShopModeCategory::Weapon:   TargetType = EItemType::Weapon;   bIsNormalShop = true; break;
	case EShopModeCategory::Armor:    TargetType = EItemType::Armor;    bIsNormalShop = true; break;
	case EShopModeCategory::Potion:   TargetType = EItemType::Potion;   bIsNormalShop = true; break;
	case EShopModeCategory::Material: TargetType = EItemType::Material; bIsNormalShop = true; break;
	case EShopModeCategory::Food:     TargetType = EItemType::Food;     bIsNormalShop = true; break;
	default: break;
	}

	// 治療屋系のカテゴリだった場合は、アイテムショップの処理を行わないのでそのまま返す
	if (!bIsNormalShop) return AvailableItems;

	// データテーブルの全行の名前（ItemID）を取得
	TArray<FName> RowNames = ItemDataTable->GetRowNames();

	// 全アイテムをループして条件に合うものを抽出
	for (const FName& RowName : RowNames)
	{
		FItemData* ItemData = ItemDataTable->FindRow<FItemData>(RowName, TEXT("ShopItemLookup"));
		if (ItemData)
		{
			// 条件1：アイテムのジャンルがショップと一致しているか
			// 条件2：アイテムのレベルがNPCの技術レベル以下か
			if (ItemData->ItemType == TargetType && ItemData->ItemLevel <= ShopLevel)
			{
				AvailableItems.Add(RowName);
			}
		}
	}

	return AvailableItems;
}

void AShopNPCBase::OpenShop(AMyProject1Character* PlayerChar)
{
	if (!PlayerChar) return;

	PlayerChar->ActiveShopNPC = this;

	FString FormattedGreeting = FString::Printf(TEXT("%s : %s"), *NPCName, *GreetingText);
	PlayerChar->OnReceiveLogMessage(FormattedGreeting, ELogMessageType::Dialogue);

	APlayerController* PC = Cast<APlayerController>(PlayerChar->GetController());
	if (!PC) return;

	AMyProject1HUD* HUD = Cast<AMyProject1HUD>(PC->GetHUD());
	if (!HUD) return;

	// --- 分岐処理の拡張 ---
	if (ShopModeCategory == EShopModeCategory::Tattoo)
	{
		HUD->ToggleTattooMenu(); // 既存
	}
	else if (ShopModeCategory == EShopModeCategory::Scar || ShopModeCategory == EShopModeCategory::Piercing || ShopModeCategory == EShopModeCategory::Disease)
	{
		HUD->ToggleTreatmentMenu(); // 既存
	}
	else
	{
		
		HUD->ToggleItemShopMenu();
	}
}


