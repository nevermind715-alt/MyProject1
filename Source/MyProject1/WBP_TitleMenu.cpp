#include "WBP_TitleMenu.h"
#include "WBP_SaveSlot.h"
#include "WBP_FadeNarration.h"
#include "MyProject1GameInstance.h"
#include "InputCoreTypes.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UWBP_TitleMenu::NativeConstruct()
{
	Super::NativeConstruct();

	// タイトルにはポーンもHUDのメニュー管理も無いので、入力モードとカーソルはこのWidget自身が面倒を見る
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}

	// NativeConstructはAddToViewportのたびに呼ばれ得るため、AddDynamicの前にRemoveDynamicして二重バインドを防ぐ
	if (Btn_NewGame)
	{
		Btn_NewGame->OnClicked.RemoveDynamic(this, &UWBP_TitleMenu::HandleNewGameClicked);
		Btn_NewGame->OnClicked.AddDynamic(this, &UWBP_TitleMenu::HandleNewGameClicked);
	}
	if (Btn_Continue)
	{
		Btn_Continue->OnClicked.RemoveDynamic(this, &UWBP_TitleMenu::HandleContinueClicked);
		Btn_Continue->OnClicked.AddDynamic(this, &UWBP_TitleMenu::HandleContinueClicked);

		// セーブデータがひとつも無ければ「つづきから」は押せない
		bool bAnySave = false;
		if (const UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance()))
		{
			for (const FSaveSlotDisplayInfo& Info : GameInst->GetAllSaveSlotInfos())
			{
				if (Info.bHasData)
				{
					bAnySave = true;
					break;
				}
			}
		}
		Btn_Continue->SetIsEnabled(bAnySave);
	}
	if (Btn_Quit)
	{
		Btn_Quit->OnClicked.RemoveDynamic(this, &UWBP_TitleMenu::HandleQuitClicked);
		Btn_Quit->OnClicked.AddDynamic(this, &UWBP_TitleMenu::HandleQuitClicked);
	}
	if (Btn_ContinueBack)
	{
		Btn_ContinueBack->OnClicked.RemoveDynamic(this, &UWBP_TitleMenu::HandleContinueBackClicked);
		Btn_ContinueBack->OnClicked.AddDynamic(this, &UWBP_TitleMenu::HandleContinueBackClicked);
	}

	ShowMainPanel();

	if (Img_IntroBackground) Img_IntroBackground->SetVisibility(ESlateVisibility::Collapsed);

	if (TitleBGM && !TitleBGMComponent)
	{
		TitleBGMComponent = UGameplayStatics::CreateSound2D(this, TitleBGM);
		if (TitleBGMComponent)
		{
			TitleBGMComponent->Play();
		}
	}
}

void UWBP_TitleMenu::NativeDestruct()
{
	if (TitleBGMComponent)
	{
		TitleBGMComponent->Stop();
		TitleBGMComponent = nullptr;
	}

	Super::NativeDestruct();
}

void UWBP_TitleMenu::ShowMainPanel()
{
	if (Panel_Main) Panel_Main->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (Panel_Continue) Panel_Continue->SetVisibility(ESlateVisibility::Collapsed);
}

FReply UWBP_TitleMenu::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIntroActive && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		AdvanceIntro();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWBP_TitleMenu::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bIntroActive)
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::SpaceBar || Key == EKeys::Enter)
		{
			AdvanceIntro();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWBP_TitleMenu::HandleNewGameClicked()
{
	if (NewGameLevel.IsNull() || bIntroActive) return;

	if (IntroPages.Num() > 0 && IntroNarrationClass)
	{
		BeginIntro();
		return;
	}

	StartNewGame();
}

void UWBP_TitleMenu::BeginIntro()
{
	IntroNarrationWidget = CreateWidget<UWBP_FadeNarration>(GetOwningPlayer(), IntroNarrationClass);
	if (!IntroNarrationWidget)
	{
		StartNewGame();
		return;
	}

	bIntroActive = true;
	IntroPageIndex = 0;

	if (Panel_Main) Panel_Main->SetVisibility(ESlateVisibility::Collapsed);
	if (Panel_Continue) Panel_Continue->SetVisibility(ESlateVisibility::Collapsed);
	if (Img_IntroBackground) Img_IntroBackground->SetVisibility(ESlateVisibility::Visible);

	// あらすじのウィジェットはこのウィジェットより手前に表示されるため、クリックを素通りさせて
	// NativeOnMouseButtonDown（このウィジェット）で受け取る。自身はVisibleにして、何もない場所のクリックも拾えるようにする
	IntroNarrationWidget->AddToViewport(10);
	IntroNarrationWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	IntroNarrationWidget->ShowLine(IntroPages[IntroPageIndex]);

	SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();
}

void UWBP_TitleMenu::AdvanceIntro()
{
	++IntroPageIndex;
	if (IntroPages.IsValidIndex(IntroPageIndex))
	{
		IntroNarrationWidget->ShowLine(IntroPages[IntroPageIndex]);
		return;
	}

	// 全ページ読み終えたので、通常のニューゲームを開始する（二重に呼ばれないよう先にフラグを下ろす）
	bIntroActive = false;
	StartNewGame();
}

void UWBP_TitleMenu::StartNewGame()
{
	RestoreGameInput();
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, NewGameLevel);
}

void UWBP_TitleMenu::RestoreGameInput()
{
	// 入力モードはビューポート側に残り、OpenLevelで新しいPlayerControllerになっても引き継がれる。
	// ここで戻しておかないと、ゲームレベルがUI専用モード・カーソル表示のまま始まりWASDが効かない。
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void UWBP_TitleMenu::HandleContinueClicked()
{
	BuildSlotList();

	if (Panel_Main) Panel_Main->SetVisibility(ESlateVisibility::Collapsed);
	if (Panel_Continue) Panel_Continue->SetVisibility(ESlateVisibility::Visible);
}

void UWBP_TitleMenu::HandleContinueBackClicked()
{
	ShowMainPanel();
}

void UWBP_TitleMenu::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UWBP_TitleMenu::BuildSlotList()
{
	if (!Scroll_Slots || !SaveSlotWidgetClass) return;

	Scroll_Slots->ClearChildren();

	const UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst) return;

	for (const FSaveSlotDisplayInfo& Info : GameInst->GetAllSaveSlotInfos())
	{
		UWBP_SaveSlot* Row = CreateWidget<UWBP_SaveSlot>(this, SaveSlotWidgetClass);
		if (!Row) continue;

		Row->Setup(Info);
		Row->SetLoadOnly();
		Row->OnLoadClicked.AddDynamic(this, &UWBP_TitleMenu::HandleLoadRequested);

		Scroll_Slots->AddChild(Row);
	}
}

void UWBP_TitleMenu::HandleLoadRequested(const FString& SlotName)
{
	UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetGameInstance());
	if (!GameInst || !GameInst->DoesSaveGameExist(SlotName)) return;

	// 内部でOpenLevelされる。タイトルにはポーズやHUDメニューの後始末が無いので、入力モードを戻すだけでよい
	RestoreGameInput();
	GameInst->LoadSavedGame(SlotName);
}
