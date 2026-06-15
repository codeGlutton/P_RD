#include "UI/SettingsPanelTypes.h"

float RDSettingsPanel::NormalizeVolumeValue(float Value)
{
	return FMath::Clamp(Value, 0.0f, 1.0f);
}

int32 RDSettingsPanel::ToQualityRequestValue(ESettingsQualityLevel QualityLevel)
{
	return StaticCast<int32>(QualityLevel);
}

FSettingsPanelValueModel RDSettingsPanel::SanitizeValueModel(const FSettingsPanelValueModel& ValueModel)
{
	FSettingsPanelValueModel SanitizedValueModel = ValueModel;
	SanitizedValueModel.mBgmVolume = NormalizeVolumeValue(SanitizedValueModel.mBgmVolume);
	SanitizedValueModel.mSfxVolume = NormalizeVolumeValue(SanitizedValueModel.mSfxVolume);
	SanitizedValueModel.mUiVolume = NormalizeVolumeValue(SanitizedValueModel.mUiVolume);
	return SanitizedValueModel;
}
