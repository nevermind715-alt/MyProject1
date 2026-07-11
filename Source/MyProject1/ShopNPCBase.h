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

public:
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

	/** アイテムデータテーブルの参照（エディタの詳細パネルから DT_Items をセットする） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ShopNPC|Data")
	UDataTable* ItemDataTable;

	/** 現在のNPCのカテゴリとレベルに基づいて、販売可能なItemID（RowName）の一覧を動的に返す関数 */
	UFUNCTION(BlueprintCallable, Category = "ShopNPC|Data")
	TArray<FName> GetAvailableShopItems() const;

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
};