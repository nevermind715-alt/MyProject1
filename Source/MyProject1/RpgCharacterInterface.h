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

	virtual void OnReceiveLogMessage(const FString& Message, ELogMessageType MessageType) = 0;

	virtual bool HasFlag(FName FlagName) const = 0;

	virtual void CancelTarget() = 0;

	virtual void NotifyStatsChanged() = 0;

	virtual void AddFlag(FName FlagName) = 0;
	virtual void RemoveFlag(FName FlagName) = 0;

	virtual struct FCharacterStats& GetCharacterStats() = 0;

	virtual class UQuestComponent* GetQuestComponent() const = 0;

	virtual void SetInputLocked(bool bLocked) = 0;

	virtual bool SetAdventurerRank(FName TargetRankRowName) = 0;
};
