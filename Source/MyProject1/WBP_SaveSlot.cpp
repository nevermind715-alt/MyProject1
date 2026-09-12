#include "WBP_SaveSlot.h"

void UWBP_SaveSlot::NativeConstruct()
{
	Super::NativeConstruct();

	// WBP_SaveMenu::RefreshSlotListが呼ばれるたびに新しいインスタンスが作られるため、
	// 現状はNativeConstructが複数回走ることは無い想定だが、WBP_SaveMenu側と同じ理由で
	// 念のためRemoveDynamicしてからAddDynamicし、将来インスタンスが使い回される変更が入っても
	// バインドが重複しないようにしておく。
	if (Btn_Save)
	{
		Btn_Save->OnClicked.RemoveDynamic(this, &UWBP_SaveSlot::HandleSaveButtonClicked);
		Btn_Save->OnClicked.AddDynamic(this, &UWBP_SaveSlot::HandleSaveButtonClicked);
	}
	if (Btn_Load)
	{
		Btn_Load->OnClicked.RemoveDynamic(this, &UWBP_SaveSlot::HandleLoadButtonClicked);
		Btn_Load->OnClicked.AddDynamic(this, &UWBP_SaveSlot::HandleLoadButtonClicked);
	}
}

void UWBP_SaveSlot::Setup(const FSaveSlotDisplayInfo& Info)
{
	SlotInfo = Info;

	if (Txt_SlotLabel)
	{
		const FText Label = Info.bIsAutoSave
			? NSLOCTEXT("WBP_SaveSlot", "AutoSaveLabel", "オートセーブ")
			: FText::Format(NSLOCTEXT("WBP_SaveSlot", "ManualSlotLabel", "スロット {0}"), FText::AsNumber(Info.SlotIndex));
		Txt_SlotLabel->SetText(Label);
	}

	if (Txt_Empty)
	{
		Txt_Empty->SetVisibility(Info.bHasData ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	// データがあるスロットの時だけ、プレイヤー情報・日付・セーブ日時を表示する
	const ESlateVisibility DetailVisibility = Info.bHasData ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (Txt_PlayerLine)
	{
		Txt_PlayerLine->SetVisibility(DetailVisibility);

		FFormatOrderedArguments Args;
		Args.Add(FText::FromString(Info.PlayerName));
		Args.Add(FText::AsNumber(Info.PlayerLevel));
		Args.Add(Info.RankText);
		Txt_PlayerLine->SetText(FText::Format(NSLOCTEXT("WBP_SaveSlot", "PlayerLineFormat", "{0}  Lv.{1} / {2}"), Args));
	}

	if (Txt_DateLine)
	{
		Txt_DateLine->SetVisibility(DetailVisibility);

		// 年は表示しない（月/日と通算経過日数のみ）
		FFormatOrderedArguments Args;
		Args.Add(FText::AsNumber(Info.InGameMonth));
		Args.Add(FText::AsNumber(Info.InGameDay));
		Args.Add(FText::AsNumber(Info.TotalElapsedDays));
		Txt_DateLine->SetText(FText::Format(NSLOCTEXT("WBP_SaveSlot", "DateLineFormat", "ゲーム内 {0}/{1}（{2}日目）"), Args));
	}

	if (Txt_SavedAt)
	{
		Txt_SavedAt->SetVisibility(DetailVisibility);
		Txt_SavedAt->SetText(FText::FromString(Info.SavedAtText));
	}

	// オートセーブ枠には手動セーブボタンを出さない
	if (Btn_Save)
	{
		Btn_Save->SetVisibility(Info.bIsAutoSave ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	// データが無いスロットはロードできない
	if (Btn_Load)
	{
		Btn_Load->SetIsEnabled(Info.bHasData);
	}
}

void UWBP_SaveSlot::HandleSaveButtonClicked()
{
	OnSaveClicked.Broadcast(SlotInfo.SlotName);
}

void UWBP_SaveSlot::HandleLoadButtonClicked()
{
	OnLoadClicked.Broadcast(SlotInfo.SlotName);
}
