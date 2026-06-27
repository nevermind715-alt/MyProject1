#include "SkinOverlayComponent.h"
#include "MyProject1Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

USkinOverlayComponent::USkinOverlayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

void USkinOverlayComponent::AddOverlay(FName OverlayRowName, float CustomOpacity, FName ShopCategory)
{
	if (OverlayRowName.IsNone()) return;

	// 【核心：重複回避】カテゴリ名とRow名を合体させた固有キーを作成（例: "Piercing_1"）
	FName UniqueKey = FName(*FString::Printf(TEXT("%s_%s"), *ShopCategory.ToString(), *OverlayRowName.ToString()));

	// フラグ箱（Map）には重複しないユニークキーで保管
	FActiveSkinOverlayState& State = ActiveOverlays.FindOrAdd(UniqueKey);
	State.RowName = OverlayRowName;

	// 見た目の上書きが必要な刺青・傷跡データテーブル検索は、本来の純粋な OverlayRowName ("1") を使用
	if (OverlayDataTable && (ShopCategory == FName("Tattoo") || ShopCategory == FName("Scar") || ShopCategory.IsNone()))
	{
		FSkinOverlayDataRow* Data = OverlayDataTable->FindRow<FSkinOverlayDataRow>(OverlayRowName, TEXT("AddOverlayLookup"));
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
}

void USkinOverlayComponent::RemoveOverlay(FName OverlayRowName, FName ShopCategory)
{
	if (OverlayRowName.IsNone()) return;

	// 消去時も合体キーで正確に対象を狙い撃つ
	FName UniqueKey = FName(*FString::Printf(TEXT("%s_%s"), *ShopCategory.ToString(), *OverlayRowName.ToString()));

	if (OverlayDataTable && (ShopCategory == FName("Tattoo") || ShopCategory == FName("Scar") || ShopCategory.IsNone()))
	{
		FSkinOverlayDataRow* Data = OverlayDataTable->FindRow<FSkinOverlayDataRow>(OverlayRowName, TEXT("RemoveOverlayLookup"));
		if (Data)
		{
			UMaterialInstanceDynamic* MID = GetBodyOverlayMID();
			if (MID)
			{
				MID->SetScalarParameterValue(Data->OpacityParamName, 0.0f);
			}
		}
	}

	if (ActiveOverlays.Contains(UniqueKey))
	{
		ActiveOverlays.Remove(UniqueKey);
	}
}

bool USkinOverlayComponent::IsOverlayActive(FName OverlayRowName, FName ShopCategory) const
{
	// 判定時も合体キーで探すため、他のカテゴリのID "1" につられる誤作動が100%起きなくなります
	FName UniqueKey = FName(*FString::Printf(TEXT("%s_%s"), *ShopCategory.ToString(), *OverlayRowName.ToString()));
	return ActiveOverlays.Contains(UniqueKey);
}

void USkinOverlayComponent::FadeOutOverlay(FName OverlayRowName, float Duration)
{
	RemoveOverlay(OverlayRowName, NAME_None);
}

void USkinOverlayComponent::ClearAllOverlays()
{
	TArray<FName> ActiveKeys;
	ActiveOverlays.GetKeys(ActiveKeys);

	for (const FName& Key : ActiveKeys)
	{
		// キー文字列（例: "Piercing_1"）から本来のRow名を取り出して消去
		FString KeyStr = Key.ToString();
		FString CategoryStr, RowNameStr;
		if (KeyStr.Split(TEXT("_"), &CategoryStr, &RowNameStr))
		{
			RemoveOverlay(FName(*RowNameStr), FName(*CategoryStr));
		}
	}
}

void USkinOverlayComponent::RefreshBodyMaterials()
{
	// 衣服変更やセーブロード時、合体キーをバラしてカテゴリとRow名を完全復元して再適用
	for (const auto& Pair : ActiveOverlays)
	{
		FString KeyStr = Pair.Key.ToString();
		FString CategoryStr, RowNameStr;
		if (KeyStr.Split(TEXT("_"), &CategoryStr, &RowNameStr))
		{
			AddOverlay(Pair.Value.RowName, Pair.Value.CurrentOpacity, FName(*CategoryStr));
		}
	}
}