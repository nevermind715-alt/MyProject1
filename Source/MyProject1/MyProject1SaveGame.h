#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MyProject1Types.h"
#include "MyProject1SaveGame.generated.h"

// セーブデータ1件分の中身。すべてUObjectポインタを含まないプレーンな構造体/配列/マップのみで構成し、
// SaveGameToSlot/LoadGameFromSlotでそのままシリアライズできるようにする。
UCLASS()
class MYPROJECT1_API UMyProject1SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// --- 復帰先の座標 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Location")
	FName PlayerLevelName;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Location")
	FTransform PlayerTransform;

	// --- キャラクターステータス ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Character")
	FCharacterStats PlayerStats;

	// --- 装備 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Equipment")
	TMap<EEquipmentSlot, FName> EquippedItems;

	// --- 所持品 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Inventory")
	TArray<FInventorySlot> InventoryContent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Inventory")
	int32 Gil = 0;

	// --- クエスト進行 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Quest")
	TArray<FQuestProgress> ActiveQuests;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Quest")
	TArray<FCompletedQuestInfo> CompletedQuests;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Quest")
	TArray<FName> EverCompletedQuestIDs;

	// --- 傷・タトゥー・ピアス・病気 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|SkinOverlay")
	TMap<FName, FActiveSkinOverlayState> ActiveTattoos;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|SkinOverlay")
	TMap<FName, FActiveSkinOverlayState> ActiveScars;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|SkinOverlay")
	TMap<FName, FActiveSkinOverlayState> ActivePiercings;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|SkinOverlay")
	TMap<FName, FActiveSkinOverlayState> ActiveDiseases;

	// --- 暦・時間 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 CurrentTimeInMinutes = 480;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 CurrentYear = 2026;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 CurrentMonth = 4;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 CurrentDay = 11;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Time")
	int32 TotalElapsedDays = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Save|Time")
	ECycleState CurrentCycleState = ECycleState::StateA;
};
