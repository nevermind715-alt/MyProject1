#include "WarpPortal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "MyProject1GameInstance.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

AWarpPortal::AWarpPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	// 当たり判定のボックスを作成
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	// 箱の大きさを設定（少し大きめに）
	CollisionBox->InitBoxExtent(FVector(100.f, 100.f, 100.f));

	// 衝突設定（プレイヤーのみに反応させるのが理想ですが、ここでは一般的な設定に）
	CollisionBox->SetCollisionProfileName(TEXT("Trigger"));

	// 触れた時のイベントを登録
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AWarpPortal::OnOverlapBegin);
}

void AWarpPortal::BeginPlay()
{
	Super::BeginPlay();
}

void AWarpPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// アクタがキャラクター（プレイヤー）かどうかチェック
	if (ACharacter* PlayerChar = Cast<ACharacter>(OtherActor))
	{
		// 「触れると即ワープ」の設定なら、すぐに飛ぶ
		if (PortalType == EWarpPortalType::TouchToWarp)
		{
			ExecuteWarp(PlayerChar);
		}
		// ※InteractToWarp（調べる）や ConfirmToWarp の場合はここでは何もしない（BPからのInteractWithPortalを待つ）
	}
}

void AWarpPortal::InteractWithPortal(ACharacter* Interactor)
{
	// Eキー等で調べられた時
	if (PortalType == EWarpPortalType::InteractToWarp)
	{
		ExecuteWarp(Interactor);
	}
}

void AWarpPortal::ExecuteWarp(ACharacter* TargetCharacter)
{
	if (!TargetCharacter || TargetWarpID.IsNone()) return;

	// 総支配人（GameInstance）を呼んでワープを依頼する
	if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
	{
		GameInst->RequestWarp(TargetWarpID, TargetCharacter);
	}
}