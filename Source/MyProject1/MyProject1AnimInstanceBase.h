// MyProject1AnimInstanceBase.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyProject1AnimInstanceBase.generated.h"

class AMyProject1Character;
class UCharacterMovementComponent;

/**
 * AMyProject1Character（プレイヤー・NPC共通の親クラス）を所有するSkeletalMeshComponentのための
 * AnimInstance共通基底クラス。
 *
 * Blueprint側で「TryGetPawnOwner→特定サブクラスへCast」を毎回自前で組むと、
 * サブクラスが変わる（プレイヤー用とNPC用でBlueprintクラスが違う）たびに
 * キャストが失敗して参照がNoneのまま固まる事故が起きやすい。
 * ここでは共通の親クラスAMyProject1Character宛にキャストすることで、
 * このAnimInstanceを使うBlueprintクラス（プレイヤー/NPC問わず）全てで
 * CharacterとMovementComponentの解決を保証する。
 */
UCLASS()
class MYPROJECT1_API UMyProject1AnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** 所有キャラクターへの参照。プレイヤー・NPCどちらの場合もここに解決される。 */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<AMyProject1Character> Character;

	/** Characterから取得したMovementComponentのキャッシュ。 */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

private:
	/**
	 * CharacterとMovementComponentの解決を試みる。
	 * AIが憑依するPawnはNativeInitializeAnimationの時点では
	 * まだPawn Ownerが確定していない場合があるため、
	 * 未解決の間はNativeUpdateAnimationから毎フレーム呼び直す。
	 */
	void ResolveOwningCharacter();
};
