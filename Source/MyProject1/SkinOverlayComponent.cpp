#include "SkinOverlayComponent.h"
#include "MyProject1Character.h"
#include "ShopNPCBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

USkinOverlayComponent::USkinOverlayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	TattooDataTable = nullptr;
	ScarDataTable = nullptr;
	PiercingDataTable = nullptr;
	DiseaseDataTable = nullptr;
	OverlayDataTable = nullptr;
}

void USkinOverlayComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AMyProject1Character>(GetOwner());
}

UMaterialInstanceDynamic* USkinOverlayComponent::GetBodyOverlayMID()
{
	if (!OwnerCharacter) return nullptr;
	USkeletalMeshComponent* BodyMesh = OwnerCharacter->GetMesh();
	if (!BodyMesh) return nullptr;

	UMaterialInterface* BaseMat = BodyMesh->GetMaterial(0);
	if (!BaseMat) return nullptr;

	UMaterialInstanceDynamic* DynamicMat = Cast<UMaterialInstanceDynamic>(BaseMat);
	if (!DynamicMat)
	{
		DynamicMat = BodyMesh->CreateDynamicMaterialInstance(0);
	}
	return DynamicMat;
}

// 列挙型による安全な「箱」の切り替えに改良
TMap<FName, FActiveSkinOverlayState>* USkinOverlayComponent::GetTargetBox(EShopModeCategory ShopCategory)
{
	switch (ShopCategory)
	{
	case EShopModeCategory::Tattoo:   return &ActiveTattoos;
	case EShopModeCategory::Scar:     return &ActiveScars;
	case EShopModeCategory::Piercing: return &ActivePiercings;
	case EShopModeCategory::Disease:  return &ActiveDiseases;
	default:                          return &ActiveTattoos;
	}
}

const TMap<FName, FActiveSkinOverlayState>* USkinOverlayComponent::GetTargetBox(EShopModeCategory ShopCategory) const
{
	switch (ShopCategory)
	{
	case EShopModeCategory::Tattoo:   return &ActiveTattoos;
	case EShopModeCategory::Scar:     return &ActiveScars;
	case EShopModeCategory::Piercing: return &ActivePiercings;
	case EShopModeCategory::Disease:  return &ActiveDiseases;
	default:                          return &ActiveTattoos;
	}
}

// 列挙型による安全なデータテーブルの切り替えに改良
UDataTable* USkinOverlayComponent::GetOverlayDataTableByCategory(EShopModeCategory ShopCategory) const
{
	switch (ShopCategory)
	{
	case EShopModeCategory::Tattoo:   return TattooDataTable ? TattooDataTable : OverlayDataTable;
	case EShopModeCategory::Scar:     return ScarDataTable ? ScarDataTable : OverlayDataTable;
	case EShopModeCategory::Piercing: return PiercingDataTable;
	case EShopModeCategory::Disease:  return DiseaseDataTable;
	default:                          return OverlayDataTable ? OverlayDataTable : TattooDataTable;
	}
}

void USkinOverlayComponent::AddOverlay(FName OverlayRowName, float CustomOpacity, EShopModeCategory ShopCategory)
{
	if (OverlayRowName.IsNone()) return;

	TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
	if (!TargetBox) return;

	FActiveSkinOverlayState& State = TargetBox->FindOrAdd(OverlayRowName);
	State.RowName = OverlayRowName;

	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);
	if (!TargetDT) return;

	UMaterialInstanceDynamic* MID = GetBodyOverlayMID();

	// ★型安全リファクタリング：カテゴリごとに正しいデータテーブル構造体を開く
	if (ShopCategory == EShopModeCategory::Tattoo)
	{
		FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(OverlayRowName, TEXT("AddOverlayLookup"));
		if (Data && MID)
		{
			UTexture2D* LoadedTex = Data->OverlayTexture.LoadSynchronous();
			if (LoadedTex)
			{
				float FinalOpacity = (CustomOpacity >= 0.0f) ? CustomOpacity : Data->DefaultOpacity;
				State.CurrentOpacity = FinalOpacity;
				MID->SetTextureParameterValue(Data->TextureParamName, LoadedTex);
				MID->SetVectorParameterValue(Data->ColorParamName, Data->ColorMultiplier);
				MID->SetScalarParameterValue(Data->OpacityParamName, FinalOpacity);
			}
		}
	}
	else if (ShopCategory == EShopModeCategory::Scar)
	{
		// 新設した傷跡専用の構造体から安全にテクスチャをロード
		FScarDataRow* Data = TargetDT->FindRow<FScarDataRow>(OverlayRowName, TEXT("AddOverlayLookup"));
		if (Data && MID)
		{
			UTexture2D* LoadedTex = Data->OverlayTexture.LoadSynchronous();
			if (LoadedTex)
			{
				float FinalOpacity = (CustomOpacity >= 0.0f) ? CustomOpacity : Data->DefaultOpacity;
				State.CurrentOpacity = FinalOpacity;
				MID->SetTextureParameterValue(Data->TextureParamName, LoadedTex);
				MID->SetVectorParameterValue(Data->ColorParamName, Data->ColorMultiplier);
				MID->SetScalarParameterValue(Data->OpacityParamName, FinalOpacity);
			}
		}
	}
	else
	{
		// 表示テクスチャがない病気やピアス
		State.CurrentOpacity = (CustomOpacity >= 0.0f) ? CustomOpacity : 1.0f;
	}
}

void USkinOverlayComponent::RemoveOverlay(FName OverlayRowName, EShopModeCategory ShopCategory)
{
	if (OverlayRowName.IsNone()) return;

	TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
	if (!TargetBox) return;

	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);
	UMaterialInstanceDynamic* MID = GetBodyOverlayMID();

	if (TargetDT && MID)
	{
		if (ShopCategory == EShopModeCategory::Tattoo)
		{
			FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(OverlayRowName, TEXT("RemoveOverlayLookup"));
			if (Data) MID->SetScalarParameterValue(Data->OpacityParamName, 0.0f);
		}
		else if (ShopCategory == EShopModeCategory::Scar)
		{
			FScarDataRow* Data = TargetDT->FindRow<FScarDataRow>(OverlayRowName, TEXT("RemoveOverlayLookup"));
			if (Data) MID->SetScalarParameterValue(Data->OpacityParamName, 0.0f);
		}
	}

	if (TargetBox->Contains(OverlayRowName))
	{
		TargetBox->Remove(OverlayRowName);
	}
}

bool USkinOverlayComponent::IsOverlayActive(FName OverlayRowName, EShopModeCategory ShopCategory) const
{
	const TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
	if (!TargetBox) return false;
	return TargetBox->Contains(OverlayRowName);
}

void USkinOverlayComponent::FadeOutOverlay(FName OverlayRowName, float Duration)
{
	RemoveOverlay(OverlayRowName, EShopModeCategory::Tattoo);
}

void USkinOverlayComponent::ClearAllOverlays()
{
	TArray<EShopModeCategory> Categories = { EShopModeCategory::Tattoo, EShopModeCategory::Scar, EShopModeCategory::Piercing, EShopModeCategory::Disease };

	for (const EShopModeCategory& Cat : Categories)
	{
		TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(Cat);
		if (TargetBox)
		{
			TArray<FName> Keys;
			TargetBox->GetKeys(Keys);
			for (const FName& Key : Keys)
			{
				RemoveOverlay(Key, Cat);
			}
		}
	}
}

void USkinOverlayComponent::RefreshBodyMaterials()
{
	TArray<EShopModeCategory> Categories = { EShopModeCategory::Tattoo, EShopModeCategory::Scar, EShopModeCategory::Piercing, EShopModeCategory::Disease };

	for (const EShopModeCategory& Cat : Categories)
	{
		TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(Cat);
		if (TargetBox)
		{
			for (const auto& Pair : *TargetBox)
			{
				AddOverlay(Pair.Value.RowName, Pair.Value.CurrentOpacity, Cat);
			}
		}
	}
}

TArray<FOverlayShopItemInfo> USkinOverlayComponent::GetGenerateShopItemList(EShopModeCategory ShopCategory) const
{
	TArray<FOverlayShopItemInfo> OutList;
	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);
	if (!TargetDT) return OutList;

	// 現在会話中のNPCの技術レベルを取得（NPCがいないデバッグ呼び出し等の場合は制限なしの99とする）
	int32 CurrentShopLevel = 99;
	if (OwnerCharacter && OwnerCharacter->ActiveShopNPC)
	{
		CurrentShopLevel = OwnerCharacter->ActiveShopNPC->ShopLevel;
	}

	TArray<FName> TargetRowNames;

	// 病気と傷跡は、自分が現在かかっているアクティブなIDだけを抽出
	if (ShopCategory == EShopModeCategory::Disease || ShopCategory == EShopModeCategory::Scar)
	{
		const TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
		if (TargetBox)
		{
			TargetBox->GetKeys(TargetRowNames);
		}
	}
	else
	{
		// 売り物（タトゥー・ピアス）はカタログ全件スキャン
		TargetRowNames = TargetDT->GetRowNames();
	}

	for (const FName& RowName : TargetRowNames)
	{
		// 一時保存用のポインタと、レベル適合確認フラグを準備
		bool bLevelRequirementMet = true;
		FOverlayShopItemInfo ItemInfo;
		ItemInfo.RowName = RowName;
		ItemInfo.bIsOwned = IsOverlayActive(RowName, ShopCategory);

		if (ShopCategory == EShopModeCategory::Tattoo)
		{
			FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(RowName, TEXT("ShopListLookup"));
			if (Data)
			{
				// 職人のレベルがタトゥーの要求レベル未満ならリストに入れない！
				if (Data->ItemLevel > CurrentShopLevel) continue;

				ItemInfo.DisplayName = Data->DisplayName;
				ItemInfo.Description = Data->Description;
				ItemInfo.BuyPrice = Data->BuyPrice;
				ItemInfo.RemovePrice = Data->RemovePrice;
			}
		}
		else if (ShopCategory == EShopModeCategory::Piercing)
		{
			FPiercingDataRow* Data = TargetDT->FindRow<FPiercingDataRow>(RowName, TEXT("ShopListLookup"));
			if (Data)
			{
				// 職人のレベルがピアスの要求レベル未満ならリストに入れない！
				if (Data->ItemLevel > CurrentShopLevel) continue;

				ItemInfo.DisplayName = Data->DisplayName;
				ItemInfo.Description = Data->Description;
				ItemInfo.BuyPrice = Data->BuyPrice;
				ItemInfo.RemovePrice = Data->RemovePrice;
			}
		}
		else if (ShopCategory == EShopModeCategory::Scar)
		{
			FScarDataRow* Data = TargetDT->FindRow<FScarDataRow>(RowName, TEXT("ShopListLookup"));
			if (Data)
			{
				// 名医のレベルが傷跡の要求レベル未満ならリストに入れない！
				if (Data->ItemLevel > CurrentShopLevel) continue;

				ItemInfo.DisplayName = Data->DisplayName;
				ItemInfo.Description = Data->Description;
				ItemInfo.BuyPrice = 0;
				ItemInfo.RemovePrice = Data->RemovePrice;
			}
		}
		else if (ShopCategory == EShopModeCategory::Disease)
		{
			FDiseaseTreatmentDataRow* Data = TargetDT->FindRow<FDiseaseTreatmentDataRow>(RowName, TEXT("ShopListLookup"));
			if (Data)
			{
				// 医者のレベルが病気の要求レベル未満ならリストに入れない！
				if (Data->ItemLevel > CurrentShopLevel) continue;

				ItemInfo.DisplayName = Data->DisplayName;
				ItemInfo.Description = Data->Description;
				ItemInfo.BuyPrice = 0;
				ItemInfo.RemovePrice = Data->TreatmentPrice;
			}
		}

		OutList.Add(ItemInfo);
	}

	return OutList;
}