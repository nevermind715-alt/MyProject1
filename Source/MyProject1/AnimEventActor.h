#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AnimEventActor.generated.h"

/**
 * FAnimEventPairingで指定された追加参加者を一時的に表示するためだけの軽量Character。
 * UMyProject1GameInstanceのアニメーションイベント再生（PlayAnimSequenceEvent）専用に、
 * FAnimSequenceEntry::ExtraPairingsのParticipantIDが初めて登場したStepでスポーンされ、イベント終了時に破棄される。
 * AI・Collision・移動は持たず、Montage再生対象（GetMesh）としてのみ使う。
 */
UCLASS()
class MYPROJECT1_API AAnimEventActor : public ACharacter
{
	GENERATED_BODY()

public:
	AAnimEventActor();
};
