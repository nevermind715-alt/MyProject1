#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h"
#include "SkinOverlayComponent.generated.h"

class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API USkinOverlayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkinOverlayComponent();

protected:
	virtual void BeginPlay() override;

	// =========================================================
	// [新設] カテゴリごとに個別に分離されたデータテーブル参照枠
	// =========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin Overlay|Data")
	UDataTable* TattooDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin Overlay|Data")
	UDataTable* ScarDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin Overlay|Data")
	UDataTable* PiercingDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin Overlay|Data")
	UDataTable* DiseaseDataTable;

	/** 旧仕様との互換維持用の予備ポインタ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin Overlay|Data")
	UDataTable* OverlayDataTable;

	// =========================================================
	// [新設] カテゴリごとに完全に分離された独立した「箱（ストレージ）」
	// =========================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skin Overlay|Storage")
	TMap<FName, FActiveSkinOverlayState> ActiveTattoos;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skin Overlay|Storage")
	TMap<FName, FActiveSkinOverlayState> ActiveScars;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skin Overlay|Storage")
	TMap<FName, FActiveSkinOverlayState> ActivePiercings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skin Overlay|Storage")
	TMap<FName, FActiveSkinOverlayState> ActiveDiseases;

private:
	/** 親アクター（AMyProject1Character）のキャッシュポインタ */
	UPROPERTY()
	class AMyProject1Character* OwnerCharacter;

	/** キャラクターの肌マテリアルスロットから動的マテリアル(MID)を安全に取得・作成する内部関数 */
	UMaterialInstanceDynamic* GetBodyOverlayMID();

public:
	/** 各カテゴリの箱（TMap）へのポインタを動的に切り替える内部ヘルパー */
	TMap<FName, FActiveSkinOverlayState>* GetTargetBox(FName ShopCategory);
	const TMap<FName, FActiveSkinOverlayState>* GetTargetBox(FName ShopCategory) const;

	/** カテゴリに応じた適切なデータテーブルを返す関数 */
	UFUNCTION(BlueprintPure, Category = "Skin Overlay")
	UDataTable* GetOverlayDataTableByCategory(FName ShopCategory) const;

	/** 箱にIDを追加し、素体メッシュにオーバーレイを適用（有効化）する */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void AddOverlay(FName OverlayRowName, float CustomOpacity = -1.0f, FName ShopCategory = NAME_None);

	/** 箱からIDを即座に削除し、マテリアルの不透明度を0にして非表示にする */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void RemoveOverlay(FName OverlayRowName, FName ShopCategory = NAME_None);

	/** 指定した時間をかけて消去する（互換維持用） */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void FadeOutOverlay(FName OverlayRowName, float Duration);

	/** すべての傷・タトゥー・化粧・ピアス・病気をそれぞれの箱からクリアする */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void ClearAllOverlays();

	/** 現在のアクティブなオーバーレイを再適用する（衣服変更時やセーブロード用） */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void RefreshBodyMaterials();

	/** 現在指定したカテゴリの箱に入っているか（有効化されているか）をチェックする関数 */
	UFUNCTION(BlueprintPure, Category = "Skin Overlay")
	bool IsOverlayActive(FName OverlayRowName, FName ShopCategory = NAME_None) const;

	UFUNCTION(BlueprintPure, Category = "Skin Overlay|Shop")
	TArray<FOverlayShopItemInfo> GetGenerateShopItemList(FName ShopCategory) const;

	const TMap<FName, FActiveSkinOverlayState>& GetActiveTattoos() const { return ActiveTattoos; }
	const TMap<FName, FActiveSkinOverlayState>& GetActiveScars() const { return ActiveScars; }
	const TMap<FName, FActiveSkinOverlayState>& GetActivePiercings() const { return ActivePiercings; }
	const TMap<FName, FActiveSkinOverlayState>& GetActiveDiseases() const { return ActiveDiseases; }

	const TMap<FName, FActiveSkinOverlayState>& GetActiveOverlays() const { return ActiveTattoos; }
	UDataTable* GetOverlayDataTable() const { return OverlayDataTable ? OverlayDataTable : TattooDataTable; }
};