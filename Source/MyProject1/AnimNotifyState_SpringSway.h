#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_SpringSway.generated.h"

/**
 * 通知区間のあいだ、Post Process ABP内のVRM4U SpringBoneノードのgravityAddを往復させて、
 * 体の上下動などで生じる慣性力を疑似的にかけ、髪・胸などの揺れ骨を強制的に揺らす。
 * 通知が終わると（Montageが途中で止まった場合も）gravityAddは元の値へ戻る。通常時の設定は変えない。
 * 全ての揺れ骨に一括で効く（骨ごとの指定はできない）。
 */
UCLASS(meta = (DisplayName = "Spring Bone Sway"))
class MYPROJECT1_API UAnimNotifyState_SpringSway : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** 力をかける向き（正規化して使う）。ReferenceBoneが空ならメッシュ（キャラ）のローカル、指定があればそのボーンのローカル。
	 *  上下動なら(0,0,1) */
	UPROPERTY(EditAnywhere, Category = "SpringSway")
	FVector Direction = FVector(0.0f, 0.0f, 1.0f);

	/** Directionの基準にするボーン。アニメで寝たり傾いたりするポーズでは、体の向きに追従するボーン（腰・背骨など）を指定する。
	 *  空（None）、またはスケルトンに存在しない名前の場合はメッシュコンポーネントのローカル */
	UPROPERTY(EditAnywhere, Category = "SpringSway")
	FName ReferenceBone = TEXT("J_Bip_C_Hips");

	/** 力の強さ。VRM1形式のスプリングならこの値がそのまま効き、VRM0形式だと内部で100倍されるため、
	 *  VRM0形式で強すぎる場合は0.1〜1程度まで下げる */
	UPROPERTY(EditAnywhere, Category = "SpringSway", meta = (ClampMin = "0.0"))
	float Amplitude = 20.0f;

	/** 往復の回数（Hz）。アニメの上下動の周期に合わせる */
	UPROPERTY(EditAnywhere, Category = "SpringSway", meta = (ClampMin = "0.0"))
	float Frequency = 1.0f;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
