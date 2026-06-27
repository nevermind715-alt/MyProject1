#include "SkinOverlayComponent.h"
#include "MyProject1Character.h"
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

// [新設] 指定されたカテゴリに対応する「個別の箱」のアドレスを返すヘルパー
TMap<FName, FActiveSkinOverlayState>* USkinOverlayComponent::GetTargetBox(FName ShopCategory)
{
	if (ShopCategory == FName("Tattoo")) return &ActiveTattoos;
	if (ShopCategory == FName("Scar")) return &ActiveScars;
	if (ShopCategory == FName("Piercing")) return &ActivePiercings;
	if (ShopCategory == FName("Disease")) return &ActiveDiseases;
	return &ActiveTattoos; // カテゴリ未指定時はTattoo箱をデフォルトにする
}

const TMap<FName, FActiveSkinOverlayState>* USkinOverlayComponent::GetTargetBox(FName ShopCategory) const
{
	if (ShopCategory == FName("Tattoo")) return &ActiveTattoos;
	if (ShopCategory == FName("Scar")) return &ActiveScars;
	if (ShopCategory == FName("Piercing")) return &ActivePiercings;
	if (ShopCategory == FName("Disease")) return &ActiveDiseases;
	return &ActiveTattoos;
}

// [新設] 指定されたカテゴリに対応するデータテーブルを切り替えるヘルパー
UDataTable* USkinOverlayComponent::GetOverlayDataTableByCategory(FName ShopCategory) const
{
	if (ShopCategory == FName("Tattoo")) return TattooDataTable ? TattooDataTable : OverlayDataTable;
	if (ShopCategory == FName("Scar")) return ScarDataTable ? ScarDataTable : OverlayDataTable;
	if (ShopCategory == FName("Piercing")) return PiercingDataTable;
	if (ShopCategory == FName("Disease")) return DiseaseDataTable;
	return OverlayDataTable ? OverlayDataTable : TattooDataTable;
}

void USkinOverlayComponent::AddOverlay(FName OverlayRowName, float CustomOpacity, FName ShopCategory)
{
	if (OverlayRowName.IsNone()) return;

	// カテゴリに応じた個別の「箱」を取得
	TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
	if (!TargetBox) return;

	// 純粋なID（例: "1"）をキーにして、独立したそれぞれの箱に保管！これで重複しても衝突しません
	FActiveSkinOverlayState& State = TargetBox->FindOrAdd(OverlayRowName);
	State.RowName = OverlayRowName;

	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);

	// 見た目（テクスチャ）の反映処理（Tattoo または Scar の場合のみマテリアル制御を行う仕様を維持）
	if (TargetDT && (ShopCategory == FName("Tattoo") || ShopCategory == FName("Scar") || ShopCategory.IsNone()))
	{
		FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(OverlayRowName, TEXT("AddOverlayLookup"));
		if (Data)
		{
			UMaterialInstanceDynamic* MID = GetBodyOverlayMID();
			if (MID)
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
	}
	else
	{
		// 表示テクスチャがない病気やピアスのデータでも、状態管理用に不透明度（生存フラグ）をセット
		State.CurrentOpacity = (CustomOpacity >= 0.0f) ? CustomOpacity : 1.0f;
	}
}

void USkinOverlayComponent::RemoveOverlay(FName OverlayRowName, FName ShopCategory)
{
	if (OverlayRowName.IsNone()) return;

	TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
	if (!TargetBox) return;

	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);

	if (TargetDT && (ShopCategory == FName("Tattoo") || ShopCategory == FName("Scar") || ShopCategory.IsNone()))
	{
		FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(OverlayRowName, TEXT("RemoveOverlayLookup"));
		if (Data)
		{
			UMaterialInstanceDynamic* MID = GetBodyOverlayMID();
			if (MID)
			{
				MID->SetScalarParameterValue(Data->OpacityParamName, 0.0f);
			}
		}
	}

	if (TargetBox->Contains(OverlayRowName))
	{
		TargetBox->Remove(OverlayRowName);
	}
}

bool USkinOverlayComponent::IsOverlayActive(FName OverlayRowName, FName ShopCategory) const
{
	const TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
	if (!TargetBox) return false;

	// 対象の個別の箱の中にそのIDが存在するかだけを純粋にチェック
	return TargetBox->Contains(OverlayRowName);
}

void USkinOverlayComponent::FadeOutOverlay(FName OverlayRowName, float Duration)
{
	RemoveOverlay(OverlayRowName, NAME_None);
}

void USkinOverlayComponent::ClearAllOverlays()
{
	TArray<FName> Categories = { FName("Tattoo"), FName("Scar"), FName("Piercing"), FName("Disease") };

	for (const FName& Cat : Categories)
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
	TArray<FName> Categories = { FName("Tattoo"), FName("Scar"), FName("Piercing"), FName("Disease") };

	for (const FName& Cat : Categories)
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

TArray<FOverlayShopItemInfo> USkinOverlayComponent::GetGenerateShopItemList(FName ShopCategory) const
{
	TArray<FOverlayShopItemInfo> OutList;
	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);
	if (!TargetDT) return OutList;

	// データテーブルに登録されている全RowName("1", "2" など)をC++側で安全に取得
	TArray<FName> AllRowNames = TargetDT->GetRowNames();

	for (const FName& RowName : AllRowNames)
	{
		FOverlayShopItemInfo ItemInfo;
		ItemInfo.RowName = RowName;

		// 既に所持（各カテゴリ個別の箱に保管）されているかをC++側で判定！
		ItemInfo.bIsOwned = IsOverlayActive(RowName, ShopCategory);

		// 各データテーブルの固有構造体から情報を引き抜いて共通情報枠へマッピング
		if (ShopCategory == FName("Tattoo") || ShopCategory == FName("Scar") || ShopCategory.IsNone())
		{
			FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(RowName, TEXT("ShopListLookup"));
			if (Data)
			{
				ItemInfo.DisplayName = Data->DisplayName;
				ItemInfo.Description = Data->Description;
				ItemInfo.BuyPrice = Data->BuyPrice;
				ItemInfo.RemovePrice = Data->RemovePrice;
			}
		}
		else if (ShopCategory == FName("Disease"))
		{
			FDiseaseTreatmentDataRow* Data = TargetDT->FindRow<FDiseaseTreatmentDataRow>(RowName, TEXT("ShopListLookup"));
			if (Data)
			{
				ItemInfo.DisplayName = Data->DisplayName;
				ItemInfo.Description = Data->Description;
				ItemInfo.BuyPrice = Data->TreatmentPrice;
				ItemInfo.RemovePrice = 0; // 病気除去費用はないため0
			}
		}
		else if (ShopCategory == FName("Piercing"))
		{
			FPiercingDataRow* Data = TargetDT->FindRow<FPiercingDataRow>(RowName, TEXT("ShopListLookup"));
			if (Data)
			{
				ItemInfo.DisplayName = Data->DisplayName;
				ItemInfo.Description = Data->Description;
				ItemInfo.BuyPrice = Data->PiercingPrice;
				ItemInfo.RemovePrice = 0; // ピアス取り外し費用はないため0
			}
		}

		OutList.Add(ItemInfo);
	}

	return OutList;
}