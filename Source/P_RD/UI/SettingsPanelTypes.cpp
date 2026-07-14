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

// 품질 단계 → 3D 렌더 해상도 목표 짧은변. 버튼 라벨(360p/720p/1080p)과 실제 적용 값의 단일 출처.
int32 RDSettingsPanel::ToRenderResolutionHeight(ESettingsQualityLevel QualityLevel)
{
	switch (QualityLevel)
	{
	case ESettingsQualityLevel::Low:
		return 360;
	case ESettingsQualityLevel::High:
		return 1080;
	case ESettingsQualityLevel::Medium:
	default:
		return 720;
	}
}

// 저장된 짧은변 → 가장 가까운 품질 단계. 패널을 다시 열 때 선택 상태를 복원하는 용도.
ESettingsQualityLevel RDSettingsPanel::FromRenderResolutionHeight(int32 ShortSideHeight)
{
	if (ShortSideHeight <= 540)
	{
		return ESettingsQualityLevel::Low;
	}
	if (ShortSideHeight <= 900)
	{
		return ESettingsQualityLevel::Medium;
	}
	return ESettingsQualityLevel::High;
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
