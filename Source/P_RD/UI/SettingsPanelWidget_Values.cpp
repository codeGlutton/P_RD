#include "UI/SettingsPanelWidget.h"

#include "Components/CheckBox.h"
#include "Components/Slider.h"

void USettingsPanelWidget::ApplyValueModel(const FSettingsPanelValueModel& ValueModel)
{
	mValueModel = RDSettingsPanel::SanitizeValueModel(ValueModel);

	mIsApplyingValueModel = true;
	if (BgmVolumeSlider != nullptr)
	{
		BgmVolumeSlider->SetValue(mValueModel.mBgmVolume);
	}
	if (SfxVolumeSlider != nullptr)
	{
		SfxVolumeSlider->SetValue(mValueModel.mSfxVolume);
	}
	if (UiVolumeSlider != nullptr)
	{
		UiVolumeSlider->SetValue(mValueModel.mUiVolume);
	}
	if (ScreenShakeCheckBox != nullptr)
	{
		ScreenShakeCheckBox->SetIsChecked(mValueModel.mScreenShakeEnabled);
	}
	if (VibrationCheckBox != nullptr)
	{
		VibrationCheckBox->SetIsChecked(mValueModel.mVibrationEnabled);
	}
	mIsApplyingValueModel = false;
}

FSettingsPanelValueModel USettingsPanelWidget::GetValueModel() const
{
	return mValueModel;
}

/**
 * @brief 설정 초기화 입력을 외부 이벤트로 전달한다.
 *
 * @details
 * 어떤 항목을 초기화할지, 초기화 직후 저장할지, 사용자에게 확인을 받을지는 설정 시스템 정책이다.
 * 이 위젯은 Reset 버튼 클릭을 OnResetRequested 요청으로만 변환한다.
 */
void USettingsPanelWidget::HandleResetButtonClicked()
{
	OnResetRequested.Broadcast();
}

/**
 * @brief 낮음 품질 선택 요청을 외부 이벤트로 전달한다.
 *
 * @details
 * 현재 패널의 임시 품질 매핑은 낮음=0, 중간=1, 높음=2다.
 * 숫자를 실제 Scalability 품질이나 프로젝트 옵션으로 해석하는 일은 이벤트 수신자가 담당한다.
 */
void USettingsPanelWidget::HandleLowQualityButtonClicked()
{
	mValueModel.mQualityLevel = ESettingsQualityLevel::Low;
	if (mIsApplyingValueModel == false)
	{
		OnQualityRequested.Broadcast(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel));
	}
}

/**
 * @brief 중간 품질 선택 요청을 외부 이벤트로 전달한다.
 *
 * @details
 * 현재 패널의 임시 품질 매핑은 낮음=0, 중간=1, 높음=2다.
 * 이 함수는 중간 품질을 뜻하는 1만 전달하고, 실제 적용 정책은 바깥에 둔다.
 */
void USettingsPanelWidget::HandleMediumQualityButtonClicked()
{
	mValueModel.mQualityLevel = ESettingsQualityLevel::Medium;
	if (mIsApplyingValueModel == false)
	{
		OnQualityRequested.Broadcast(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel));
	}
}

/**
 * @brief 높음 품질 선택 요청을 외부 이벤트로 전달한다.
 *
 * @details
 * 현재 패널의 임시 품질 매핑은 낮음=0, 중간=1, 높음=2다.
 * 이 함수는 높음 품질을 뜻하는 2만 전달하고, 플랫폼별 품질 제한은 바깥 정책에서 처리한다.
 */
void USettingsPanelWidget::HandleHighQualityButtonClicked()
{
	mValueModel.mQualityLevel = ESettingsQualityLevel::High;
	if (mIsApplyingValueModel == false)
	{
		OnQualityRequested.Broadcast(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel));
	}
}

/**
 * @brief BGM 볼륨 변경 값을 외부 이벤트로 전달한다.
 *
 * @details
 * Slider 값은 UI 입력 값 그대로 전달한다.
 * 실제 사운드 믹스, 저장값 변환, 음소거 정책은 OnBgmVolumeChanged를 받은 시스템에서 처리한다.
 */
void USettingsPanelWidget::HandleBgmVolumeChanged(float Value)
{
	mValueModel.mBgmVolume = RDSettingsPanel::NormalizeVolumeValue(Value);
	if (mIsApplyingValueModel == false)
	{
		OnBgmVolumeChanged.Broadcast(mValueModel.mBgmVolume);
	}
}

/**
 * @brief 효과음 볼륨 변경 값을 외부 이벤트로 전달한다.
 *
 * @details
 * Slider 값은 UI 입력 값 그대로 전달한다.
 * 효과음 버스나 저장 설정에 어떤 방식으로 반영할지는 외부 설정/오디오 시스템의 책임이다.
 */
void USettingsPanelWidget::HandleSfxVolumeChanged(float Value)
{
	mValueModel.mSfxVolume = RDSettingsPanel::NormalizeVolumeValue(Value);
	if (mIsApplyingValueModel == false)
	{
		OnSfxVolumeChanged.Broadcast(mValueModel.mSfxVolume);
	}
}

/**
 * @brief UI 볼륨 변경 값을 외부 이벤트로 전달한다.
 *
 * @details
 * 버튼 클릭음, 알림음 같은 UI 사운드가 어떤 그룹에 묶이는지는 오디오 정책이 결정한다.
 * 이 위젯은 UiVolumeSlider 입력만 OnUiVolumeChanged로 올린다.
 */
void USettingsPanelWidget::HandleUiVolumeChanged(float Value)
{
	mValueModel.mUiVolume = RDSettingsPanel::NormalizeVolumeValue(Value);
	if (mIsApplyingValueModel == false)
	{
		OnUiVolumeChanged.Broadcast(mValueModel.mUiVolume);
	}
}

/**
 * @brief 화면 흔들림 체크 상태를 외부 이벤트로 전달한다.
 *
 * @details
 * 체크박스 값은 사용자의 선호 입력일 뿐이다.
 * 실제 카메라 흔들림을 끄거나 저장하는 처리는 OnScreenShakeChanged 수신자가 담당한다.
 */
void USettingsPanelWidget::HandleScreenShakeChanged(bool bChecked)
{
	mValueModel.mScreenShakeEnabled = bChecked;
	if (mIsApplyingValueModel == false)
	{
		OnScreenShakeChanged.Broadcast(mValueModel.mScreenShakeEnabled);
	}
}

/**
 * @brief 진동 체크 상태를 외부 이벤트로 전달한다.
 *
 * @details
 * 진동 지원 여부는 플랫폼과 입력 장치에 따라 다를 수 있다.
 * 패널은 사용자가 요청한 체크 상태만 전달하고, 지원 여부 판단과 적용은 외부 시스템에 맡긴다.
 */
void USettingsPanelWidget::HandleVibrationChanged(bool bChecked)
{
	mValueModel.mVibrationEnabled = bChecked;
	if (mIsApplyingValueModel == false)
	{
		OnVibrationChanged.Broadcast(mValueModel.mVibrationEnabled);
	}
}
