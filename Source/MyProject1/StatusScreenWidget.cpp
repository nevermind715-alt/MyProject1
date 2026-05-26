#include "StatusScreenWidget.h"
#include "MyProject1Character.h" // MyStatsを取得するために必須

void UStatusScreenWidget::UpdateAllStatus(AMyProject1Character* PlayerCharacter)
{
    // キャラクター情報が無効な場合は処理を中断
    if (!PlayerCharacter) return;

    // キャラクターのステータス構造体を取得
    const FCharacterStats& Stats = PlayerCharacter->MyStats;

    // --- テキストの更新 ---
    // FText::AsNumber() を使うことで、floatやintを自動的に適切なテキスト形式に変換します。
    // 小数点を表示したくないステータスの場合は、FMath::RoundToInt() で整数に丸めるとUIが綺麗にまとまります。

    if (Text_CharName)
    {
        // 名前は FString なので FText::FromString を使用
        FString DisplayName = Stats.NPCName.IsEmpty() ? PlayerCharacter->CharacterName : Stats.NPCName;
        Text_CharName->SetText(FText::FromString(DisplayName));
    }

    if (Text_Level)     Text_Level->SetText(FText::AsNumber(Stats.Level));
    if (Text_HP)        Text_HP->SetText(FText::AsNumber(FMath::RoundToInt(Stats.HP)));
    if (Text_ST)        Text_ST->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Stamina)));

    if (Txt_STR)        Txt_STR->SetText(FText::AsNumber(FMath::RoundToInt(Stats.STR)));
    if (Txt_VIT)        Txt_VIT->SetText(FText::AsNumber(FMath::RoundToInt(Stats.VIT)));
    if (Txt_Dex)        Txt_Dex->SetText(FText::AsNumber(FMath::RoundToInt(Stats.DEX)));
    if (Txt_AGI)        Txt_AGI->SetText(FText::AsNumber(FMath::RoundToInt(Stats.AGI)));
    if (Txt_Mental)     Txt_Mental->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Mental)));
    if (Txt_Charm)      Txt_Charm->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Charm)));

    if (Txt_Fame)       Txt_Fame->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Fame)));
    if (Txt_Favor)      Txt_Favor->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Favor)));
    if (Txt_Hostility)  Txt_Hostility->SetText(FText::AsNumber(FMath::RoundToInt(Stats.Hostility)));
}