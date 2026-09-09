#pragma once

#include "CoreMinimal.h"
#include "UI/SettingsPanelWidget.h"
#include "MobileInputTestsHelper.generated.h"

UCLASS()
class URDMobileInputTestListener : public UObject
{
	GENERATED_BODY()
public:
	int32 Taps = 0;
	int32 Holds = 0;
	int32 Backs = 0;
	UFUNCTION() void WorldTouch(FVector2D Position, bool LongPress) { if (LongPress) ++Holds; else ++Taps; }
	UFUNCTION() void Back() { ++Backs; }
};

/** Isolates the modal predicate from the commandlet's absent game viewport. */
UCLASS()
class URDMobileSettingsTestWidget : public USettingsPanelWidget
{
	GENERATED_BODY()
public:
	bool TestOpened = false;
	virtual bool IsOpened() const override { return TestOpened; }
};
