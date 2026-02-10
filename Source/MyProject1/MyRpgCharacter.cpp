// Fill out your copyright notice in the Description page of Project Settings.


#include "MyRpgCharacter.h"

// Sets default values
AMyRpgCharacter::AMyRpgCharacter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMyRpgCharacter::BeginPlay()
{
	Super::BeginPlay();
	
    
    FString DebugMessage = FString::Printf(TEXT("Spawned: %s / HP: %d"), *CharacterName, CurrentHP);

    // 画面左上にシアン色（水色）で10秒間表示する
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, DebugMessage);
    }

    // ★ここまで追加★
}

// Called every frame
void AMyRpgCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// ★一番下に追加
void AMyRpgCharacter::ReceiveDamage(int32 DamageAmount)
{
    // HPを減らす
    CurrentHP = CurrentHP - DamageAmount;

    // HPが0以下にならないようにする
    if (CurrentHP < 0)
    {
        CurrentHP = 0;
    }

    // デバッグ表示（誰かがダメージを受けたか分かるように）
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("%s took %d damage! HP: %d"), *CharacterName, DamageAmount, CurrentHP);
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Msg);
    }

    // ここに死亡判定などを後で追加できます
}

