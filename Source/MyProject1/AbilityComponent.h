#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // データテーブルの構造体を使うために追加
#include "AbilityComponent.generated.h"

// ホットバー（1〜0キー）の割り当てが変わった時にUIへ知らせる合図
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHotbarChangedSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- データテーブル ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	class UDataTable* AbilityDataTable;

	// --- アビリティの習得と管理 ---
	// キャラクターが現在覚えているアビリティIDリスト
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Ability")
	TArray<FName> LearnedAbilities;

	// リキャスト（クールダウン）中のアビリティリスト（ID -> 残り秒数）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability")
	TMap<FName, float> RecastTimers;

	// 新しいアビリティを覚える関数
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void LearnAbility(FName AbilityID);

	// アビリティを習得させる（未習得の時だけ）。bShowLogなら「習得した！」のログを出す。新たに習得した時だけtrueを返す（C++専用）
	bool GrantAbility(FName AbilityID, bool bShowLog);

	// 習得レベル（FAbilityData::LearnLevel）がLevel以下で、まだ習得していないアビリティを全て習得させる（C++専用）
	void GrantAbilitiesForLevel(int32 Level, bool bShowLog);

	// --- ホットバー（キーボードの1〜0キーに割り当てたアビリティ） ---
	// スロット数。インデックス0=キー1 ... 8=キー9, 9=キー0
	static constexpr int32 NumHotbarSlots = 10;

	// 各スロットに割り当てたアビリティID（未割り当てはNAME_None）。常にNumHotbarSlots個ある
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Hotbar")
	TArray<FName> HotbarSlots;

	// 割り当てが変わった時に通知（HUDのホットバー表示の再描画用）
	UPROPERTY(BlueprintAssignable, Category = "Ability|Hotbar")
	FOnHotbarChangedSignature OnHotbarChanged;

	// スロットにアビリティを割り当てる。習得済みのものだけ可。同じアビリティが別スロットにあれば、そちらは空にする（移動扱い）
	UFUNCTION(BlueprintCallable, Category = "Ability|Hotbar")
	bool AssignHotbarSlot(int32 SlotIndex, FName AbilityID);

	// スロットを空にする
	UFUNCTION(BlueprintCallable, Category = "Ability|Hotbar")
	void ClearHotbarSlot(int32 SlotIndex);

	// スロットのアビリティを発動する（中身はTryCastAbility。ターゲットは現在のロックオン対象）
	UFUNCTION(BlueprintCallable, Category = "Ability|Hotbar")
	bool UseHotbarSlot(int32 SlotIndex);

	// セーブデータからの復元用。習得済み一覧とホットバーをまとめて差し替える（C++専用）
	void RestoreFromSave(const TArray<FName>& InLearnedAbilities, const TArray<FName>& InHotbarSlots);

	// アビリティIDからデータテーブルの行を引く（UI表示用。見つからなければnullptr）
	const FAbilityData* FindAbilityData(FName AbilityID) const;

	// リキャストの残り秒数（クールタイム中でなければ0）
	float GetRecastRemaining(FName AbilityID) const;

	// --- 詠唱（キャスト）の管理 ---
	// 現在詠唱中かどうか
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Casting")
	bool bIsCasting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Casting")
	class UNiagaraComponent* ActiveCastEffectComponent = nullptr;

	// 現在詠唱しているアビリティのID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Casting")
	FName CurrentCastingAbilityID;

	// 魔法の発動対象（ターゲット）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability|Casting")
	AActor* CurrentTarget;

	// アビリティを使おうと試みる関数（TP/MP不足やリキャスト中なら失敗する）
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool TryCastAbility(FName AbilityID, AActor* TargetActor);

	// 詠唱を中断する関数（ダメージを受けた時や動いた時などに呼ぶ）
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void CancelCasting();

private:
	// 詠唱タイマーの管理用
	FTimerHandle CastTimerHandle;

	// 詠唱完了時に実際に効果を発動させる関数
	void ExecuteAbility();
};
