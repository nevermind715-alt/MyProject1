#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyProject1Types.h"
#include "ShopNPCBase.generated.h"

UCLASS()
class MYPROJECT1_API AShopNPCBase : public APawn
{
	GENERATED_BODY()

public:
	AShopNPCBase();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** ゲーム内時間帯による表示切り替え（例：18時〜22時だけ開いている店）。時間帯の外では店員が消えて、話しかけられなくなる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Time Visibility")
	FActiveTimeWindow ActiveTimeWindow;

	// =========================================================
	// 見た目（メッシュ・アニメーション）
	// =========================================================

	/** NPCの見た目本体。レベルに配置したインスタンスごとにメッシュを個別設定できる */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ShopNPC|Mesh")
	class USkeletalMeshComponent* NPCMesh;

	/** 複雑な動き（ステートマシン等）が必要な場合に設定するAnimBlueprint。設定するとIdleAnimationより優先される */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Mesh")
	TSubclassOf<class UAnimInstance> NPCAnimClass;

	/** AnimBlueprintを使わない場合に、単純にループ再生させるアニメーションシーケンス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Mesh")
	class UAnimSequence* IdleAnimation;

	// =========================================================
	// 自由に変えられるNPCの固有パラメーター（詳細パネル用）
	// =========================================================

	/** ログに表示する店員の名前（例: 彫師アニキ、街の医者） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Display")
	FString NPCName;

	/** C++側に送るショップの識別カテゴリ（ドロップダウンで安全に選択） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data")
	EShopModeCategory ShopModeCategory;

	/** この職人・医者の技術レベル（この数値未満のItemLevelの呪物・高等治療を扱える） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data", meta = (ClampMin = "1"))
	int32 ShopLevel;

	/** ピアス医者ショップ（ShopModeCategory=Piercing）の装着タブで、DT_Itemsの価格に上乗せする施術料。店ごとに調整可 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data", meta = (ClampMin = "0"))
	int32 PiercingAddMarkup = 10000;

	/** アイテムデータテーブルの参照（エディタの詳細パネルから DT_Items をセットする） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data")
	UDataTable* ItemDataTable;

	/** trueの場合、非合法アイテム（FItemData::bIsIllegal）をプレイヤーへ販売できる（店頭に並ぶ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data")
	bool bSellsIllegalItems = false;

	/** trueの場合、非合法アイテム（FItemData::bIsIllegal）をプレイヤーから買い取れる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data")
	bool bBuysIllegalItems = false;

	/** trueの場合、ShopModeCategory/ShopLevelによる自動判定を無視し、SpecificItemsに登録したアイテムのみを扱う */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data")
	bool bUseSpecificItemsOnly = false;

	/** bUseSpecificItemsOnlyがtrueの時だけ使う、個別アイテムの取扱リスト（購入可/売却可を個別指定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data", meta = (EditCondition = "bUseSpecificItemsOnly"))
	TArray<FShopItemOverride> SpecificItems;

	/** 現在のNPCのカテゴリとレベル（または特定アイテムリスト）に基づいて、販売可能なItemID（RowName）の一覧を動的に返す関数 */
	UFUNCTION(BlueprintCallable, Category = "ShopNPC|Data")
	TArray<FName> GetAvailableShopItems() const;

	/** このNPCからItemIDを購入できるかを判定する */
	UFUNCTION(BlueprintCallable, Category = "ShopNPC|Data")
	bool CanBuyItem(FName ItemID) const;

	/** このNPCへItemIDを売却できるかを判定する */
	UFUNCTION(BlueprintCallable, Category = "ShopNPC|Data")
	bool CanSellItem(FName ItemID) const;

	/** ItemIDが非合法アイテム（FItemData::bIsIllegal）かどうかをItemDataTableから調べる */
	UFUNCTION(BlueprintPure, Category = "ShopNPC|Data")
	bool IsItemIllegal(FName ItemID) const;

	/** Blueprint（WBP）側でそのままFName（"Tattoo"等）として引き出せるようにする自動変換関数 */
	UFUNCTION(BlueprintPure, Category = "ShopNPC|Data")
	FName GetShopModeCategoryAsName() const;

	/** 話しかけた時に生成して画面に表示するWidget（WBP_TattooShopなど） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|UI")
	TSubclassOf<class UUserWidget> ShopWidgetClass;

	/** 話しかけた時の最初のセリフ（いらっしゃいませ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Dialogue")
	FString GreetingText;

	/** お店を閉じた時の見送りのセリフ（またどうぞ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Dialogue")
	FString GoodbyeText;

	// =========================================================
	// インタラクト時の共通ロジック関数
	// =========================================================

	/** BPI_InteractableからEキーで呼び出す、ショップ開店の共通ネイティブ処理 */
	UFUNCTION(BlueprintCallable, Category = "ShopNPC|Actions")
	void OpenShop(class AMyProject1Character* PlayerChar);

private:
	/** ゲーム内時間が進むたびに呼ばれる（OnInGameTimeChangedの購読先）。時間帯の内外が変わったら表示/当たり判定を更新する */
	UFUNCTION()
	void OnInGameTimeChangedHandler(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute);

	/** bInWindow（時間帯の中か）に応じて表示/当たり判定を反映する */
	void ApplyTimeWindowVisibility(bool bInWindow);
};