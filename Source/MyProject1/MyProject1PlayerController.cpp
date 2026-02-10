// Copyright Epic Games, Inc. All Rights Reserved.


#include "MyProject1PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AMyProject1PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMyProject1, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMyProject1PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AMyProject1PlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// SetupInputComponent 内にバインドを追加（Spaceキーのアクションを事前に作成しておく必要があります）
void AMyProject1PlayerController::OpenCommandMenu()
{
	// すでに開いていたら何もしない
	if (CommandMenuInstance) return;

	if (CommandMenuWidgetClass)
	{
		CommandMenuInstance = CreateWidget<UUserWidget>(this, CommandMenuWidgetClass);
		if (CommandMenuInstance)
		{
			CommandMenuInstance->AddToViewport();

			// FF11のようにメニュー操作中はマウスカーソルを出す設定
			SetInputMode(FInputModeGameAndUI());
			bShowMouseCursor = true;
		}
	}
}