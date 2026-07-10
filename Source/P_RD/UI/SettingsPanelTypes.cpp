// 설정 값 검증/변환 헬퍼 구현. 위젯이 값 정리를 직접 하지 않고 여기 한곳에 모아 재사용.
#include "UI/SettingsPanelTypes.h"

// 슬라이더가 0~1 밖의 값을 줘도 안전하게 자른다.
float RDSettingsPanel::NormalizeVolumeValue(float Value)
{
	return FMath::Clamp(Value, 0.0f, 1.0f);
}

// 품질 enum(Low/Medium/High) → 정수(0/1/2). 품질 적용 요청에 그대로 넘긴다.
int32 RDSettingsPanel::ToQualityRequestValue(ESettingsQualityLevel QualityLevel)
{
	return StaticCast<int32>(QualityLevel);
}

// 값 모델 전체를 안전 범위로 정리(모든 볼륨 0~1). 저장/적용 전에 한 번 거치는 용도.
FSettingsPanelValueModel RDSettingsPanel::SanitizeValueModel(const FSettingsPanelValueModel& ValueModel)
{
	FSettingsPanelValueModel SanitizedValueModel = ValueModel;
	SanitizedValueModel.mBgmVolume = NormalizeVolumeValue(SanitizedValueModel.mBgmVolume);
	SanitizedValueModel.mSfxVolume = NormalizeVolumeValue(SanitizedValueModel.mSfxVolume);
	SanitizedValueModel.mUiVolume = NormalizeVolumeValue(SanitizedValueModel.mUiVolume);
	SanitizedValueModel.mMasterVolume = NormalizeVolumeValue(SanitizedValueModel.mMasterVolume);
	// FPS는 지원 값(30/60)으로 스냅한다.
	SanitizedValueModel.mFpsLimit = SanitizedValueModel.mFpsLimit <= 30 ? 30 : 60;
	return SanitizedValueModel;
}
