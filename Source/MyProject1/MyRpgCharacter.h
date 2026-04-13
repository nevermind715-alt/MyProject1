// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyRpgCharacter.generated.h"

UCLASS()
class MYPROJECT1_API AMyRpgCharacter : public ACharacter
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyRpgCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	// キャラクターの名前
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Status")
	FString CharacterName = "Senshi";

	// 最大HP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Status")
	int32 MaxHP = 200;

	// 現在のHP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Status")
	int32 CurrentHP = 200;

	// 攻撃力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Status")
	int32 AttackPower = 50;

	// 防御力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Status")
	int32 DefensePower = 10;

	// ダメージを受ける関数
	// BlueprintCallable: ブループリントから「ダメージ受けろ！」と命令できる
	UFUNCTION(BlueprintCallable, Category = "RPG Combat")
	void ReceiveDamage(int32 DamageAmount);

	// 命中・クリティカルに関わる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Status")
	int32 DEX = 20;

	// 回避・防御に関わる
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG Status")
	int32 AGI = 20;

	
};
