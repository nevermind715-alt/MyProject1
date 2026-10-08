#include "WBP_CommandMenu.h"
#include "MyProject1Character.h"
#include "MyProject1HUD.h"
#include "GameFramework/PlayerController.h"

namespace
{
	AMyProject1HUD* GetOwnerHUD(const UUserWidget* Widget)
	{
		APlayerController* PC = Widget->GetOwningPlayer();
		return PC ? PC->GetHUD<AMyProject1HUD>() : nullptr;
	}

	AMyProject1Character* GetOwnerCharacter(const UUserWidget* Widget)
	{
		APlayerController* PC = Widget->GetOwningPlayer();
		return PC ? Cast<AMyProject1Character>(PC->GetPawn()) : nullptr;
	}
}

void UWBP_CommandMenu::NativeConstruct()
{
	Super::NativeConstruct();

	// NativeConstructはAddToViewportのたびに毎回呼ばれる（コマンドメニューはHUD側で使い回される）ため、
	// AddDynamicの前にRemoveDynamicしておかないと、開閉のたびにバインドが積み重なり1クリックで複数回呼ばれてしまう。
	if (Btn_Stats)
	{
		Btn_Stats->OnClicked.RemoveDynamic(this, &UWBP_CommandMenu::HandleStatsClicked);
		Btn_Stats->OnClicked.AddDynamic(this, &UWBP_CommandMenu::HandleStatsClicked);
	}
	if (Btn_Save)
	{
		Btn_Save->OnClicked.RemoveDynamic(this, &UWBP_CommandMenu::HandleSaveClicked);
		Btn_Save->OnClicked.AddDynamic(this, &UWBP_CommandMenu::HandleSaveClicked);
	}
	if (Btn_Quest)
	{
		Btn_Quest->OnClicked.RemoveDynamic(this, &UWBP_CommandMenu::HandleQuestClicked);
		Btn_Quest->OnClicked.AddDynamic(this, &UWBP_CommandMenu::HandleQuestClicked);
	}
	if (Btn_Item)
	{
		Btn_Item->OnClicked.RemoveDynamic(this, &UWBP_CommandMenu::HandleItemClicked);
		Btn_Item->OnClicked.AddDynamic(this, &UWBP_CommandMenu::HandleItemClicked);
	}
	if (Btn_Equ)
	{
		Btn_Equ->OnClicked.RemoveDynamic(this, &UWBP_CommandMenu::HandleEquClicked);
		Btn_Equ->OnClicked.AddDynamic(this, &UWBP_CommandMenu::HandleEquClicked);
	}
	if (Btn_Abl)
	{
		Btn_Abl->OnClicked.RemoveDynamic(this, &UWBP_CommandMenu::HandleAbilityClicked);
		Btn_Abl->OnClicked.AddDynamic(this, &UWBP_CommandMenu::HandleAbilityClicked);
	}
	if (Btn_Attack)
	{
		Btn_Attack->OnClicked.RemoveDynamic(this, &UWBP_CommandMenu::HandleAttackClicked);
		Btn_Attack->OnClicked.AddDynamic(this, &UWBP_CommandMenu::HandleAttackClicked);

		// 文字用のTextBlockに変数名が付いていない場合は、ボタンの子から探す
		if (!Txt_Fight)
		{
			Txt_Fight = Cast<UTextBlock>(Btn_Attack->GetChildAt(0));
		}
	}

	RefreshState();
}

void UWBP_CommandMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 元のBlueprintバインド関数（Get_Fight_Text / Get_Btn_*_bIsEnabled）は毎フレーム評価されていたため、同じ頻度で反映する
	RefreshState();
}

void UWBP_CommandMenu::RefreshState()
{
	AMyProject1Character* Character = GetOwnerCharacter(this);
	if (!Character) return;

	if (Txt_Fight)
	{
		const bool bInCombat = Character->bIsAutoAttacking || Character->IsPreparingAttack();
		Txt_Fight->SetText(bInCombat
			? NSLOCTEXT("WBP_CommandMenu", "FightStop", "戦闘停止")
			: NSLOCTEXT("WBP_CommandMenu", "FightStart", "戦闘開始"));
	}

	// 死亡中は戦闘・アイテム・アビリティ・装備を押せない
	const bool bAlive = !Character->IsDead();
	if (Btn_Attack) Btn_Attack->SetIsEnabled(bAlive);
	if (Btn_Item) Btn_Item->SetIsEnabled(bAlive);
	if (Btn_Abl) Btn_Abl->SetIsEnabled(bAlive);
	if (Btn_Equ) Btn_Equ->SetIsEnabled(bAlive);
}

void UWBP_CommandMenu::HandleStatsClicked()
{
	if (AMyProject1HUD* HUD = GetOwnerHUD(this)) HUD->ToggleStatusMenu();
}

void UWBP_CommandMenu::HandleSaveClicked()
{
	if (AMyProject1HUD* HUD = GetOwnerHUD(this)) HUD->ToggleSaveMenu();
}

void UWBP_CommandMenu::HandleQuestClicked()
{
	if (AMyProject1HUD* HUD = GetOwnerHUD(this)) HUD->ToggleQuestMenu();
}

void UWBP_CommandMenu::HandleItemClicked()
{
	if (AMyProject1HUD* HUD = GetOwnerHUD(this)) HUD->ToggleInventoryMenu();
}

void UWBP_CommandMenu::HandleEquClicked()
{
	if (AMyProject1HUD* HUD = GetOwnerHUD(this)) HUD->ToggleEquipmentMenu();
}

void UWBP_CommandMenu::HandleAbilityClicked()
{
	if (AMyProject1HUD* HUD = GetOwnerHUD(this)) HUD->ToggleAbilityMenu();
}

void UWBP_CommandMenu::HandleAttackClicked()
{
	if (AMyProject1Character* Character = GetOwnerCharacter(this)) Character->ToggleCombatMode();
}
