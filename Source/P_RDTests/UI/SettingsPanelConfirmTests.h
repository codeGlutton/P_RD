#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SettingsPanelConfirmTests.generated.h"

UCLASS()
class USettingsPanelConfirmTestListener : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION() void HandleSave() { ++SaveCount; }
	UFUNCTION() void HandleAbandon() { ++AbandonCount; }
	int32 SaveCount = 0;
	int32 AbandonCount = 0;
};
