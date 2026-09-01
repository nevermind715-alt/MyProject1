#include "SkinOverlayComponent.h"
#include "MyProject1Character.h"
#include "MyProject1GameInstance.h"
#include "QuestNPCBase.h"
#include "ShopNPCBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"
#include "TimerManager.h"

// Canvas描画でしか使わないオーバーレイ用テクスチャは、通常の「画面上のメッシュに貼られて見える」ルートで
// ストリーミングマネージャーに検知されないため、初回だけ低解像度のまま焼き込まれてしまう。
// ストリーミング対象から完全に外すことで、以後は常に全ミップ常駐の状態で描画できるようにする（初回のみ実処理）。
static void EnsureOverlayTextureFullyResident(UTexture2D* Texture)
{
	if (!Texture || Texture->NeverStream) return;

	Texture->NeverStream = true;
	Texture->UpdateResource();
	Texture->WaitForStreaming();
}

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

	// レベル移動でこのコンポーネントが再生成された場合、GameInstanceに記憶しておいた
	// 前回の中身（傷・タトゥー・ピアス・病気）を4つの箱に読み戻す。
	LoadOverlayStateFromGameInstance();

	// Play開始直後・レベルワープ直後はレンダーパイプラインが温まっていないことがあるため、
	// RenderReadyDelay秒はRefreshBodyMaterialsの実処理を保留する。
	bIsRenderReady = false;
	bHasPendingRefresh = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle_RenderReady, this, &USkinOverlayComponent::HandleRenderReadyTimer, RenderReadyDelay, false);
	}

	// 読み戻した中身を実際にメッシュへ反映させる（実処理はRenderReadyDelay経過後に保留実行される）
	RefreshBodyMaterials();
}

void USkinOverlayComponent::HandleRenderReadyTimer()
{
	bIsRenderReady = true;

	if (bHasPendingRefresh)
	{
		bHasPendingRefresh = false;
		ExecuteRefreshBodyMaterials();
	}
}

void USkinOverlayComponent::SaveOverlayStateToGameInstance()
{
	// GameInstance側のSaved***は「Player 1体分」を前提にしたシングルトンの記憶領域。
	// QuestNPCもこのコンポーネントを持つため、ここを区別せずに使うとNPCがタトゥーを
	// 付けるたびにPlayerの記憶領域を上書きしてしまい、後でPlayer側にNPCのタトゥーが
	// 漏れて表示される不具合を起こす。NPCは対象外にする。
	if (Cast<AQuestNPCBase>(OwnerCharacter)) return;

	UWorld* World = GetWorld();
	UMyProject1GameInstance* GameInst = World ? World->GetGameInstance<UMyProject1GameInstance>() : nullptr;
	if (!GameInst) return;

	GameInst->SavedActiveTattoos = ActiveTattoos;
	GameInst->SavedActiveScars = ActiveScars;
	GameInst->SavedActivePiercings = ActivePiercings;
	GameInst->SavedActiveDiseases = ActiveDiseases;
}

void USkinOverlayComponent::LoadOverlayStateFromGameInstance()
{
	// 保存側と同じ理由で、NPCはPlayer専用の記憶領域を読み込まない。
	if (Cast<AQuestNPCBase>(OwnerCharacter)) return;

	UWorld* World = GetWorld();
	UMyProject1GameInstance* GameInst = World ? World->GetGameInstance<UMyProject1GameInstance>() : nullptr;
	if (!GameInst) return;

	ActiveTattoos = GameInst->SavedActiveTattoos;
	ActiveScars = GameInst->SavedActiveScars;
	ActivePiercings = GameInst->SavedActivePiercings;
	ActiveDiseases = GameInst->SavedActiveDiseases;
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

	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);
	if (!TargetDT) return;

	// 箱にデータを追加・記憶する
	FActiveSkinOverlayState& State = TargetBox->FindOrAdd(OverlayRowName);
	State.RowName = OverlayRowName;

	// デフォルト不透明度の取得
	float FinalOpacity = 1.0f;
	if (ShopCategory == EShopModeCategory::Tattoo)
	{
		if (FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(OverlayRowName, TEXT("AddLookup")))
			FinalOpacity = Data->DefaultOpacity;
	}
	else if (ShopCategory == EShopModeCategory::Scar)
	{
		if (FScarDataRow* Data = TargetDT->FindRow<FScarDataRow>(OverlayRowName, TEXT("AddLookup")))
			FinalOpacity = Data->DefaultOpacity;
	}

	State.CurrentOpacity = (CustomOpacity >= 0.0f) ? CustomOpacity : FinalOpacity;

	// レベルを移動しても消えないよう、GameInstance側にも同じ中身を記憶させる
	SaveOverlayStateToGameInstance();

	// ★記憶が終わったら、自動連番ロジックでマテリアルを最新状態に一斉更新！
	RefreshBodyMaterials();

	// 装備と同じ仕組みで、罹患・装着中のStatModifiersをステータスに反映する
	if (OwnerCharacter)
	{
		OwnerCharacter->RefreshEquipmentStats();
	}
}

void USkinOverlayComponent::RemoveOverlay(FName OverlayRowName, EShopModeCategory ShopCategory)
{
	if (OverlayRowName.IsNone()) return;

	TMap<FName, FActiveSkinOverlayState>* TargetBox = GetTargetBox(ShopCategory);
	if (!TargetBox) return;

	// 箱からデータを削除する
	if (TargetBox->Contains(OverlayRowName))
	{
		TargetBox->Remove(OverlayRowName);

		// レベルを移動しても消えないよう、GameInstance側にも同じ中身を記憶させる
		SaveOverlayStateToGameInstance();

		// ★削除したデータを詰めてマテリアルを再描画！
		RefreshBodyMaterials();

		// 装備と同じ仕組みで、治癒・除去による補正の消滅をステータスに反映する
		if (OwnerCharacter)
		{
			OwnerCharacter->RefreshEquipmentStats();
		}
	}
}

TArray<FEquipmentStatModifier> USkinOverlayComponent::GetActiveStatModifiers() const
{
	TArray<FEquipmentStatModifier> Result;

	// タトゥー（刺青）
	if (TattooDataTable)
	{
		for (const auto& Pair : ActiveTattoos)
		{
			if (FSkinOverlayDataRow* Data = TattooDataTable->FindRow<FSkinOverlayDataRow>(Pair.Key, TEXT("TattooStatLookup")))
			{
				Result.Append(Data->StatModifiers);
			}
		}
	}

	// 傷跡
	if (ScarDataTable)
	{
		for (const auto& Pair : ActiveScars)
		{
			if (FScarDataRow* Data = ScarDataTable->FindRow<FScarDataRow>(Pair.Key, TEXT("ScarStatLookup")))
			{
				Result.Append(Data->StatModifiers);
			}
		}
	}

	// ピアスはDT_Equipments（装備）へ統合済み。ステータス補正は装備側のRefreshEquipmentStatsが担当するため、
	// ここで ActivePiercings を加算すると二重加算になる。よってピアスの加算はしない（旧オーバーレイ方式の名残）。

	// 病気・ケガ
	if (DiseaseDataTable)
	{
		for (const auto& Pair : ActiveDiseases)
		{
			if (FDiseaseTreatmentDataRow* Data = DiseaseDataTable->FindRow<FDiseaseTreatmentDataRow>(Pair.Key, TEXT("DiseaseStatLookup")))
			{
				Result.Append(Data->StatModifiers);
			}
		}
	}

	return Result;
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


// --- SkinOverlayComponent.cpp の RefreshBodyMaterials を完全に置き換え ---

void USkinOverlayComponent::RefreshBodyMaterials()
{
	// レンダーパイプラインの準備待ち中は、実際の描画を1回分だけ保留する。
	// （保留中に何度呼ばれても、準備完了時にまとめて最新状態を1回だけ描画する）
	if (!bIsRenderReady)
	{
		bHasPendingRefresh = true;
		return;
	}

	ExecuteRefreshBodyMaterials();
}

void USkinOverlayComponent::ExecuteRefreshBodyMaterials()
{
	UMaterialInstanceDynamic* MID = GetBodyOverlayMID();
	if (!MID) return;

	// 1. 合成用の共通2048キャンバスがなければ、C++側で動的に生成
	if (!CombinedTattooTarget)
	{
		CombinedTattooTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), 2048, 2048, RTF_RGBA8);
	}

	// キャンバスを一度完全に透明な黒（0, 0, 0, 0）にクリア
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CombinedTattooTarget, FLinearColor(0, 0, 0, 0));

	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext DrawContext;

	// レンタターゲットへの描画（Canvas合成）を開始
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), CombinedTattooTarget, Canvas, CanvasSize, DrawContext);

	if (Canvas)
	{
		// ============================================================================
		// レイヤー①【一番下】：現在装備中の「薄手衣装（下着・タイツなど）」を等倍スタンプ
		// ============================================================================
		if (OwnerCharacter && OwnerCharacter->EquipmentDataTable)
		{
			for (const auto& Pair : OwnerCharacter->CurrentEquippedItems)
			{
				FName ItemID = Pair.Value;
				if (!ItemID.IsNone())
				{
					FEquipmentData* EquipData = OwnerCharacter->EquipmentDataTable->FindRow<FEquipmentData>(ItemID, TEXT("CanvasEquipment"));
					if (EquipData && !EquipData->OverlayTexture.IsNull())
					{
						UTexture2D* LoadedTex = EquipData->OverlayTexture.LoadSynchronous();
						if (LoadedTex)
						{
							EnsureOverlayTextureFullyResident(LoadedTex);

							// 衣服の指定色と不透明度をブレンドして等倍描画
							FLinearColor RenderColor = EquipData->ColorMultiplier;
							RenderColor.A *= EquipData->DefaultOpacity;

							Canvas->K2_DrawTexture(LoadedTex, FVector2D::ZeroVector, CanvasSize, FVector2D::ZeroVector, FVector2D::UnitVector, RenderColor, EBlendMode::BLEND_Translucent);
						}
					}
				}
			}
		}

		// ============================================================================
		// レイヤー②【真ん中】：アクティブな「傷跡（ActiveScars）」を上から等倍スタンプ
		// ============================================================================
		UDataTable* ScarDT = GetOverlayDataTableByCategory(EShopModeCategory::Scar);
		if (ScarDT)
		{
			for (const auto& Pair : ActiveScars)
			{
				FScarDataRow* Data = ScarDT->FindRow<FScarDataRow>(Pair.Value.RowName, TEXT("CanvasScars"));
				if (Data)
				{
					UTexture2D* LoadedTex = Data->OverlayTexture.LoadSynchronous();
					if (LoadedTex)
					{
						EnsureOverlayTextureFullyResident(LoadedTex);

						FLinearColor RenderColor = Data->ColorMultiplier;
						RenderColor.A *= Pair.Value.CurrentOpacity;

						Canvas->K2_DrawTexture(LoadedTex, FVector2D::ZeroVector, CanvasSize, FVector2D::ZeroVector, FVector2D::UnitVector, RenderColor, EBlendMode::BLEND_Translucent);
					}
				}
			}
		}

		// ============================================================================
		// レイヤー③【一番上】：アクティブな「刺青（ActiveTattoos）」をさらに上から等倍スタンプ
		// ============================================================================
		UDataTable* TattooDT = GetOverlayDataTableByCategory(EShopModeCategory::Tattoo);
		if (TattooDT)
		{
			for (const auto& Pair : ActiveTattoos)
			{
				FSkinOverlayDataRow* Data = TattooDT->FindRow<FSkinOverlayDataRow>(Pair.Value.RowName, TEXT("CanvasTattoos"));
				if (Data)
				{
					UTexture2D* LoadedTex = Data->OverlayTexture.LoadSynchronous();
					if (LoadedTex)
					{
						EnsureOverlayTextureFullyResident(LoadedTex);

						FLinearColor RenderColor = Data->ColorMultiplier;
						RenderColor.A *= Pair.Value.CurrentOpacity;

						Canvas->K2_DrawTexture(LoadedTex, FVector2D::ZeroVector, CanvasSize, FVector2D::ZeroVector, FVector2D::UnitVector, RenderColor, EBlendMode::BLEND_Translucent);
					}
				}
			}
		}
	}
	// 描画処理を終了して確定
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), DrawContext);

	// 衣服・傷跡・刺青のすべてが完璧なレイヤー順で美しく融合した最終的な1枚（CombinedTattooTarget）を流し込む！
	MID->SetTextureParameterValue(TEXT("Tattoo_Tex_1"), CombinedTattooTarget);
}

bool USkinOverlayComponent::PopulateShopItemInfo(UDataTable* TargetDT, FName RowName, EShopModeCategory ShopCategory, int32 CurrentShopLevel, FOverlayShopItemInfo& OutInfo) const
{
	OutInfo.RowName = RowName;
	OutInfo.bIsOwned = IsOverlayActive(RowName, ShopCategory);

	if (ShopCategory == EShopModeCategory::Tattoo)
	{
		FSkinOverlayDataRow* Data = TargetDT->FindRow<FSkinOverlayDataRow>(RowName, TEXT("ShopItemLookup"));
		// 職人のレベルがタトゥーの要求レベル未満ならリストに入れない！
		if (!Data || Data->ItemLevel > CurrentShopLevel) return false;

		OutInfo.DisplayName = Data->DisplayName;
		OutInfo.Description = Data->Description;
		OutInfo.BuyPrice = Data->BuyPrice;
		OutInfo.RemovePrice = Data->RemovePrice;
		return true;
	}
	// ピアスはDT_Equipmentsへ統合済み。GetGenerateShopItemList / GetShopItemInfoByRowName が
	// Piercingカテゴリを先に横取りしてBuildPiercingShopListへ委譲するため、ここには到達しない。
	else if (ShopCategory == EShopModeCategory::Scar)
	{
		FScarDataRow* Data = TargetDT->FindRow<FScarDataRow>(RowName, TEXT("ShopItemLookup"));
		// 名医のレベルが傷跡の要求レベル未満ならリストに入れない！
		if (!Data || Data->ItemLevel > CurrentShopLevel) return false;

		OutInfo.DisplayName = Data->DisplayName;
		OutInfo.Description = Data->Description;
		OutInfo.BuyPrice = 0;
		OutInfo.RemovePrice = Data->RemovePrice;
		return true;
	}
	else if (ShopCategory == EShopModeCategory::Disease)
	{
		FDiseaseTreatmentDataRow* Data = TargetDT->FindRow<FDiseaseTreatmentDataRow>(RowName, TEXT("ShopItemLookup"));
		// 医者のレベルが病気の要求レベル未満ならリストに入れない！
		if (!Data || Data->ItemLevel > CurrentShopLevel) return false;

		OutInfo.DisplayName = Data->DisplayName;
		OutInfo.Description = Data->Description;
		OutInfo.BuyPrice = 0;
		OutInfo.RemovePrice = Data->TreatmentPrice;
		return true;
	}

	return false;
}

TArray<FOverlayShopItemInfo> USkinOverlayComponent::GetGenerateShopItemList(EShopModeCategory ShopCategory) const
{
	TArray<FOverlayShopItemInfo> OutList;

	// ピアス・拘束具はDT_Equipments（装備）へ統合済み。装備側でリストを組み立てる。
	if (ShopCategory == EShopModeCategory::Piercing || ShopCategory == EShopModeCategory::Restraint)
	{
		if (OwnerCharacter)
		{
			OutList = (ShopCategory == EShopModeCategory::Piercing)
				? OwnerCharacter->BuildPiercingShopList()
				: OwnerCharacter->BuildRestraintShopList();
		}
		return OutList;
	}

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
		FOverlayShopItemInfo ItemInfo;
		if (PopulateShopItemInfo(TargetDT, RowName, ShopCategory, CurrentShopLevel, ItemInfo))
		{
			OutList.Add(ItemInfo);
		}
	}

	return OutList;
}

bool USkinOverlayComponent::GetShopItemInfoByRowName(FName RowName, EShopModeCategory ShopCategory, FOverlayShopItemInfo& OutInfo) const
{
	if (RowName.IsNone()) return false;

	// ピアス・拘束具はDT_Equipments（装備）へ統合済み。装備側リストから該当行を探す。
	if (ShopCategory == EShopModeCategory::Piercing || ShopCategory == EShopModeCategory::Restraint)
	{
		if (!OwnerCharacter) return false;
		const TArray<FOverlayShopItemInfo> List = (ShopCategory == EShopModeCategory::Piercing)
			? OwnerCharacter->BuildPiercingShopList()
			: OwnerCharacter->BuildRestraintShopList();
		for (const FOverlayShopItemInfo& Info : List)
		{
			if (Info.RowName == RowName)
			{
				OutInfo = Info;
				return true;
			}
		}
		return false;
	}

	UDataTable* TargetDT = GetOverlayDataTableByCategory(ShopCategory);
	if (!TargetDT) return false;

	int32 CurrentShopLevel = 99;
	if (OwnerCharacter && OwnerCharacter->ActiveShopNPC)
	{
		CurrentShopLevel = OwnerCharacter->ActiveShopNPC->ShopLevel;
	}

	return PopulateShopItemInfo(TargetDT, RowName, ShopCategory, CurrentShopLevel, OutInfo);
}