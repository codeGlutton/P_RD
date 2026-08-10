#pragma once

#include "CoreMinimal.h"

#include "SettingsRunActionTests.generated.h"

/** @brief 설정판 런 액션의 UIModel 요청/결과 채널을 검증하는 수신기. */
UCLASS()
class USettingsRunActionTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleSaveAndExitRequested();

	UFUNCTION()
	void HandleAbandonRequested();

	UFUNCTION()
	void HandleSaveAndExitCompleted(bool bSuccess);

	UFUNCTION()
	void HandleAbandonRunCompleted(bool bSuccess);

	int32 SaveRequestCount = 0;
	int32 AbandonRequestCount = 0;
	int32 CompletionCount = 0;
	bool bLastCompletionSucceeded = false;
	int32 AbandonCompletionCount = 0;
	bool bLastAbandonCompletionSucceeded = false;
};
