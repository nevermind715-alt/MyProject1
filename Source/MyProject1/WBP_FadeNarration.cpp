#include "WBP_FadeNarration.h"

void UWBP_FadeNarration::ShowLine(const FText& Line)
{
	if (!Txt_FadeNarration) return;

	Txt_FadeNarration->SetVisibility(ESlateVisibility::Visible);
	Txt_FadeNarration->SetText(Line);
}

void UWBP_FadeNarration::HideNarration()
{
	if (!Txt_FadeNarration) return;

	Txt_FadeNarration->SetVisibility(ESlateVisibility::Collapsed);
}
