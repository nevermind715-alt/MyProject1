#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h" // UTextBlockを使うために必須
#include "StatusScreenWidget.generated.h"

class AMyProject1Character; // 前方宣言

UCLASS()
class MYPROJECT1_API UStatusScreenWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    // ==========================================
    // UIコンポーネントのバインド（BPの変数名と完全一致させる）
    // ==========================================


    // --- 既存の表示項目（画像リストにはないが必須と思われるもの） ---
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Txt_CharName;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Txt_CurrentHP;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* Txt_CurrentStamina;

    // --- 基礎ステータス・戦闘力 ---
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_MaxHP;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_MaxStamina;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_AttackPower;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_DefensePower;

    // --- 能力値 (Attributes) ---
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_STR;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_VIT;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_DEX;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_AGI;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Karma;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Mental;

    // --- 社会的・隠しステータス ---
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Fame;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Favor;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Hostility;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Charm;

    // --- レベル・経験値 ---
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Level;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_NextLevelXP;

    // --- 拡張ステータス枠 (1 ～ 16) ---
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats1;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats2;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats3;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats4;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats5;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats6;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats7;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats8;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats9;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats10;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats11;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats12;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats13;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats14;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats15;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_ExStats16;

public:
    // ==========================================
    // BPから呼び出して一括更新する関数
    // ==========================================
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateAllStatus(AMyProject1Character* PlayerCharacter);
};