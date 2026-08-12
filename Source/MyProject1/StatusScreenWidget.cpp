#include "StatusScreenWidget.h"
#include "MyProject1Character.h" // MyStatsを取得するために必須

void UStatusScreenWidget::UpdateAllStatus(AMyProject1Character* PlayerCharacter)
{
    // キャラクター情報が無効な場合は処理を中断
    if (!PlayerCharacter) return;

    // キャラクターのステータス構造体を取得
    const FCharacterStats& Stats = PlayerCharacter->MyStats;

    // ==========================================
    // 各テキストブロックの更新処理
    // ==========================================

    // --- キャラクター名 ---
    if (Txt_CharName)
    {
        FString DisplayName = Stats.NPCName.IsEmpty() ? PlayerCharacter->CharacterName : Stats.NPCName;
        Txt_CharName->SetText(FText::FromString(DisplayName));
    }

    // --- 基礎ステータス・戦闘力 ---
    if (Txt_CurrentHP)      Txt_CurrentHP->SetText(FText::AsNumber(FMath::RoundToInt(Stats.HP)));
    if (Txt_CurrentStamina) Txt_CurrentStamina->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Stamina)));

    if (Txt_MaxHP)          Txt_MaxHP->SetText(FText::AsNumber(FMath::RoundToInt(Stats.MaxHP)));
    if (Txt_MaxStamina)     Txt_MaxStamina->SetText(FText::AsNumber(FMath::RoundToInt(Stats.MaxStamina)));
    if (Txt_AttackPower)    Txt_AttackPower->SetText(FText::AsNumber(FMath::RoundToInt(Stats.AttackPower)));
    if (Txt_DefensePower)   Txt_DefensePower->SetText(FText::AsNumber(FMath::RoundToInt(Stats.DefensePower)));

    // --- 能力値 (Attributes) ---
    if (Txt_STR)        Txt_STR->SetText(FText::AsNumber(FMath::RoundToInt(Stats.STR)));
    if (Txt_VIT)        Txt_VIT->SetText(FText::AsNumber(FMath::RoundToInt(Stats.VIT)));
    if (Txt_DEX)        Txt_DEX->SetText(FText::AsNumber(FMath::RoundToInt(Stats.DEX)));
    if (Txt_AGI)        Txt_AGI->SetText(FText::AsNumber(FMath::RoundToInt(Stats.AGI)));
    if (Txt_Karma)       Txt_Karma->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Karma)));
    if (Txt_Mental)     Txt_Mental->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Mental)));

    // --- 社会的・隠しステータス ---
    if (Txt_Fame)       Txt_Fame->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Fame)));
    if (Txt_Favor)      Txt_Favor->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Favor)));
    if (Txt_Hostility)  Txt_Hostility->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Hostility)));
    if (Txt_Charm)      Txt_Charm->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Charm)));
    if (Txt_Alcohol)    Txt_Alcohol->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Alcohol)));

    // --- レベル・経験値 ---
    if (Txt_Level)       Txt_Level->SetText(FText::AsNumber(Stats.Level));
    if (Txt_NextLevelXP) Txt_NextLevelXP->SetText(FText::AsNumber(FMath::Max(0, Stats.MaxXP - Stats.CurrentXP)));


    // ==========================================
    // --- 拡張枠 (Ex Stats 1 ~ 16) の動的取得 ---
    // ==========================================

    // TMap（ExtraStats）から指定したキー名で値を探してFTextに変換するラムダ関数（ヘルパー）
    auto GetExtraStatText = [](const TMap<FName, float>& ExtraStatsMap, const FString& KeyName) -> FText
        {
            // ExtraStatsのTMap内を、文字列キー（例: "Ex Stats1"）で検索します
            const float* FoundValue = ExtraStatsMap.Find(FName(*KeyName));
            if (FoundValue)
            {
                // 値が見つかった場合は、四捨五入して整数値としてテキスト化
                return FText::AsNumber(FMath::RoundToInt(*FoundValue));
            }

            // マップの中にキーが見つからなかった場合は、ハイフンを表示
            return FText::FromString(TEXT("-"));
        };

    // 各テキストブロックに対して、ExtraStatsから対応する名前のデータを引き出してセット
    if (Txt_ExStats1)  Txt_ExStats1->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats1")));
    if (Txt_ExStats2)  Txt_ExStats2->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats2")));
    if (Txt_ExStats3)  Txt_ExStats3->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats3")));
    if (Txt_ExStats4)  Txt_ExStats4->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats4")));
    if (Txt_ExStats5)  Txt_ExStats5->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats5")));
    if (Txt_ExStats6)  Txt_ExStats6->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats6")));
    if (Txt_ExStats7)  Txt_ExStats7->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats7")));
    if (Txt_ExStats8)  Txt_ExStats8->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats8")));
    if (Txt_ExStats9)  Txt_ExStats9->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats9")));
    if (Txt_ExStats10) Txt_ExStats10->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats10")));
    if (Txt_ExStats11) Txt_ExStats11->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats11")));
    if (Txt_ExStats12) Txt_ExStats12->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats12")));
    if (Txt_ExStats13) Txt_ExStats13->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats13")));
    if (Txt_ExStats14) Txt_ExStats14->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats14")));
    if (Txt_ExStats15) Txt_ExStats15->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats15")));
    if (Txt_ExStats16) Txt_ExStats16->SetText(GetExtraStatText(Stats.ExtraStats, TEXT("ExStats16")));
}