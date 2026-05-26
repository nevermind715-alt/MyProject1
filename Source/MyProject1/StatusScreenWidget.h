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

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Text_CharName;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Text_HP;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Text_Level;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Text_ST;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_AGI;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Charm;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Dex;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Fame;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Favor;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Hostility;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Mental;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_STR;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_VIT;

public:
    // ==========================================
    // BPから呼び出して一括更新する関数
    // ==========================================
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateAllStatus(AMyProject1Character* PlayerCharacter);
};