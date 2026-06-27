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

	/** 傷やタトゥーのテクスチャデータを一元管理するデータテーブル */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin Overlay|Data")
	UDataTable* OverlayDataTable;

	/** 現在このキャラクターに適用（有効化）されている固有合体キーとその状態のリスト */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skin Overlay|Storage")
	TMap<FName, FActiveSkinOverlayState> ActiveOverlays;

private:
	/** 親アクター（AMyProject1Character）のキャッシュポインタ */
	UPROPERTY()
	class AMyProject1Character* OwnerCharacter;

	/** キャラクターの肌マテリアルスロット（Element 0）から動的マテリアル(MID)を安全に取得・作成する内部関数 */
	UMaterialInstanceDynamic* GetBodyOverlayMID();

	/** カテゴリ名とRow名から、絶対に重複しない唯一無二の合体キーを自動生成・補正する内部ヘルパー関数 */
	FName GetUniqueKey(FName OverlayRowName, FName ShopCategory) const;

public:
	/** 箱にIDを追加し、素体メッシュにオーバーレイを適用（有効化）する（安全弁・隔離用の引数を追加） */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void AddOverlay(FName OverlayRowName, float CustomOpacity = -1.0f, FName ShopCategory = NAME_None);

	/** 箱からIDを即座に削除し、マテリアルの不透明度を0にして非表示にする（隔離用の引数を追加） */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void RemoveOverlay(FName OverlayRowName, FName ShopCategory = NAME_None);

	/** 指定した時間をかけて消去する（Blueprint側のエラーを防ぐ互換性維持用の関数） */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void FadeOutOverlay(FName OverlayRowName, float Duration);

	/** すべての傷・タトゥー・化粧を箱から空にして綺麗にする */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void ClearAllOverlays();

	/** 現在のアクティブなオーバーレイを再適用する（衣服変更時やセーブロード用） */
	UFUNCTION(BlueprintCallable, Category = "Skin Overlay")
	void RefreshBodyMaterials();

	/** 現在箱に入っているか（有効化されているか）をチェックする関数（★カテゴリ対応の隔離判定に変更） */
	UFUNCTION(BlueprintPure, Category = "Skin Overlay")
	bool IsOverlayActive(FName OverlayRowName, FName ShopCategory = NAME_None) const;

	/** 現在箱に入っているすべての状態を取得 */
	const TMap<FName, FActiveSkinOverlayState>& GetActiveOverlays() const { return ActiveOverlays; }

	UDataTable* GetOverlayDataTable() const { return OverlayDataTable; }
};