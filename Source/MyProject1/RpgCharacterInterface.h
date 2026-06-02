#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MyProject1Types.h"
#include "RpgCharacterInterface.generated.h"

UINTERFACE(MinimalAPI)
class URpgCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

class MYPROJECT1_API IRpgCharacterInterface
{
	GENERATED_BODY()

public:
	// --- 共通の窓口（インターフェース）の定義 ---

	// 1. システムログやダメージログを受け取る窓口
	virtual void OnReceiveLogMessage(const FString& Message, ELogMessageType MessageType) = 0;

	// 2. 特定のフラグを持っているか確認する窓口
	virtual bool HasFlag(FName FlagName) const = 0;

	// 3. 現在のターゲット状態をキャンセルする窓口
	virtual void CancelTarget() = 0;

	// 4. ステータス（MyStats）の変化をUIなどに通知する窓口
	virtual void NotifyStatsChanged() = 0;

	// 5. フラグを追加・削除する窓口
	virtual void AddFlag(FName FlagName) = 0;
	virtual void RemoveFlag(FName FlagName) = 0;

	// 6. ステータス（MyStats）を直接読み書きするための窓口
	virtual struct FCharacterStats& GetCharacterStats() = 0;

	// 7. クエストコンポーネントへの窓口
	virtual class UQuestComponent* GetQuestComponent() const = 0;

	// 8. 会話中などにプレイヤーの操作をロックする窓口
	virtual void SetInputLocked(bool bLocked) = 0;
};