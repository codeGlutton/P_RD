#pragma once

#include "RDMinimal.h"

#include "SettingsPanelTypes.generated.h"

UENUM(BlueprintType)
enum class ESettingsPanelMode : uint8
{
	Title,
	InGame,
};

UENUM(BlueprintType)
enum class ESettingsQualityLevel : uint8
{
	Low = 0,
	Medium = 1,
	High = 2,
};

USTRUCT(BlueprintType)
struct P_RD_API FSettingsPanelValueModel
{
	GENERATED_BODY()

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	float mBgmVolume = 1.0f;

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	float mSfxVolume = 1.0f;

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	float mUiVolume = 1.0f;

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	bool mScreenShakeEnabled = true;

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	bool mVibrationEnabled = true;

	UPROPERTY(Category = "Settings", EditAnywhere, BlueprintReadWrite)
	ESettingsQualityLevel mQualityLevel = ESettingsQualityLevel::Medium;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSettingsPanelEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelFloatEvent, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelBoolEvent, bool, bValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSettingsPanelIntEvent, int32, Value);

namespace RDSettingsPanel
{
	P_RD_API float NormalizeVolumeValue(float Value);
	P_RD_API int32 ToQualityRequestValue(ESettingsQualityLevel QualityLevel);
	P_RD_API FSettingsPanelValueModel SanitizeValueModel(const FSettingsPanelValueModel& ValueModel);
}
