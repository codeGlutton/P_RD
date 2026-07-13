#pragma once

#include "CoreMinimal.h"
#include "UI/Combat/CombatUIModel.h"

#include "CombatFloatingLogTests.generated.h"

UCLASS()
class UCombatFloatingLogTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleFloatingLog(FCombatFloatingLogRequest Request);

	int32 mFloatingLogCallCount = 0;
	FCombatFloatingLogRequest mLastRequest;
};
