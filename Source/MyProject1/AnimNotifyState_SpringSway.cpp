#include "AnimNotifyState_SpringSway.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

// 日本語文字化け・コンパイルエラー対策
#pragma execution_character_set("utf-8")

namespace
{
	// 通知ステートのオブジェクトはアセット共有でインスタンスごとの状態を持てないため、メッシュごとの状態はここに置く。
	// 同じメッシュに複数の通知（シーケンス側とMontage側など）が同時に効くことがあるので、通知ごとに経過時間と揺れの量を持ち、
	// gravityAddには元の値＋全通知の合計を書き込む
	struct FSpringSwayEntry
	{
		float Elapsed = 0.0f;
		FVector Offset = FVector::ZeroVector;
	};
	struct FSpringSwayState
	{
		FVector OriginalGravityAdd = FVector::ZeroVector;
		TMap<const UAnimNotifyState*, FSpringSwayEntry> Entries;

		FVector Total() const
		{
			FVector Sum = OriginalGravityAdd;
			for (const TPair<const UAnimNotifyState*, FSpringSwayEntry>& Pair : Entries)
			{
				Sum += Pair.Value.Offset;
			}
			return Sum;
		}
	};
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FSpringSwayState> GSpringSwayStates;

	// Post Process AnimInstanceが持つAnimNode_VrmSpringBoneのgravityAddを探す。
	// VRM4UのヘッダにはVRM4Uモジュールへの依存が要るため、リフレクションで名前から辿る
	FVector* FindSpringGravityAdd(USkeletalMeshComponent* MeshComp)
	{
		UAnimInstance* PostProcessInstance = MeshComp ? MeshComp->GetPostProcessInstance() : nullptr;
		if (!PostProcessInstance) return nullptr;

		for (TFieldIterator<FStructProperty> It(PostProcessInstance->GetClass()); It; ++It)
		{
			if (It->Struct->GetFName() != TEXT("AnimNode_VrmSpringBone")) continue;

			const FStructProperty* GravityAddProp = CastField<FStructProperty>(It->Struct->FindPropertyByName(TEXT("gravityAdd")));
			if (!GravityAddProp || GravityAddProp->Struct != TBaseStructure<FVector>::Get()) return nullptr;

			void* NodePtr = It->ContainerPtrToValuePtr<void>(PostProcessInstance);
			return GravityAddProp->ContainerPtrToValuePtr<FVector>(NodePtr);
		}
		return nullptr;
	}
}

void UAnimNotifyState_SpringSway::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	FVector* GravityAdd = FindSpringGravityAdd(MeshComp);
	if (!GravityAdd)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpringSway: %s のPost Process AnimInstanceにAnimNode_VrmSpringBoneが見つかりません"), *GetNameSafe(MeshComp));
		return;
	}

	// 最初の通知が始まるときだけ元の値を保存する（2つ目以降が揺れ用の値を「元の値」と取り違えないように）
	if (!GSpringSwayStates.Contains(MeshComp))
	{
		GSpringSwayStates.Add(MeshComp).OriginalGravityAdd = *GravityAdd;
	}
	GSpringSwayStates[MeshComp].Entries.Add(this);
}

void UAnimNotifyState_SpringSway::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	FSpringSwayState* State = GSpringSwayStates.Find(MeshComp);
	FSpringSwayEntry* Entry = State ? State->Entries.Find(this) : nullptr;
	FVector* GravityAdd = FindSpringGravityAdd(MeshComp);
	if (!Entry || !GravityAdd) return;

	Entry->Elapsed += FrameDeltaTime;
	const float Wave = FMath::Sin(2.0f * PI * Frequency * Entry->Elapsed);

	// Directionをローカル（ReferenceBone指定時はそのボーン、なければメッシュ）基準からワールドの向きへ変換する
	const bool bUseBone = !ReferenceBone.IsNone() && MeshComp->GetBoneIndex(ReferenceBone) != INDEX_NONE;
	const FTransform BasisTransform = bUseBone ? MeshComp->GetSocketTransform(ReferenceBone, RTS_World) : MeshComp->GetComponentTransform();
	Entry->Offset = BasisTransform.TransformVectorNoScale(Direction.GetSafeNormal()) * (Amplitude * Wave);
	*GravityAdd = State->Total();
}

void UAnimNotifyState_SpringSway::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	FSpringSwayState* State = GSpringSwayStates.Find(MeshComp);
	if (!State || State->Entries.Remove(this) == 0) return;

	if (FVector* GravityAdd = FindSpringGravityAdd(MeshComp))
	{
		*GravityAdd = State->Total();
	}
	if (State->Entries.IsEmpty())
	{
		GSpringSwayStates.Remove(MeshComp);
	}
}
