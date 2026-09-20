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
	UFUNCTION()
	void HandleEventBatch(FCombatEventBatchUI Batch);

	int32 mFloatingLogCallCount = 0;
	FCombatFloatingLogRequest mLastRequest;
	int32 mEventBatchCallCount = 0;
	FCombatEventBatchUI mLastBatch;
};
