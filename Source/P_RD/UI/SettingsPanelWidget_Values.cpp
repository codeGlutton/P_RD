#include "UI/SettingsPanelWidget.h"

#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "GameMode/RDGameModeBase.h"

#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataType.h"

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
	if (MasterVolumeSlider != nullptr)
	{
		MasterVolumeSlider->SetValue(mValueModel.mMasterVolume);
	}
	if (EffectsCheckBox != nullptr)
	{
		EffectsCheckBox->SetIsChecked(mValueModel.mEffectsEnabled);
	}
	mIsApplyingValueModel = false;
	SyncText();
	UpdateGraphicsSelectionIndicators();
}

void USettingsPanelWidget::RefreshValueModelFromCurrentOptions()
{
	FSettingsPanelValueModel CurrentValueModel = mValueModel;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UPersistentDataSubsystem* PersistentDataSubsystem = GameInstance->GetSubsystem<UPersistentDataSubsystem>())
		{
			if (const UOptionPersistData* OptionData = PersistentDataSubsystem->GetOptionPersistData())
			{
				CurrentValueModel.mMasterVolume = OptionData->GetVolume(EGameVolumeType::Master);
				CurrentValueModel.mBgmVolume = OptionData->GetVolume(EGameVolumeType::BGM);
				CurrentValueModel.mSfxVolume = OptionData->GetVolume(EGameVolumeType::SFX);
				CurrentValueModel.mUiVolume = OptionData->GetVolume(EGameVolumeType::UI);
				CurrentValueModel.mUseKoreanLanguage = OptionData->GetLanguage() == ELanguageType::KOREAN;
				CurrentValueModel.mFpsLimit = OptionData->GetFpsLimit();
				CurrentValueModel.mQualityLevel = RDSettingsPanel::FromOverallQuality(StaticCast<int32>(OptionData->GetOverallQuality()));
			}
		}
	}
	ApplyValueModel(CurrentValueModel);
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
	// 전용 수신자가 생기기 전까지의 기본 적용: 옵션을 CDO 기본값으로 되돌리고
	// (ResetOptions -> ClearOption -> ApplyCurrentOptions로 볼륨/언어/해상도 즉시 재적용),
	// 패널 UI도 기본 값 모델로 갱신한다(ApplyValueModel은 콜백 가드로 이벤트 재발신 없음).
	// 초기화도 커밋 행위다 — 되돌린 옵션을 즉시 디스크에 저장한다.
	if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
	{
		GameModeBase->ResetFromOptionPanel();
	}
	RefreshValueModelFromCurrentOptions();
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
	UpdateGraphicsSelectionIndicators();
	if (mIsApplyingValueModel == false)
	{
		OnQualityRequested.Broadcast(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel));
		// 전용 수신자가 생기기 전까지의 기본 적용: 품질 단계를 목표 렌더 해상도(360p)로 바꿔 프로필에 반영한다(FPS와 동일 패턴).
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetOverallQuality(StaticCast<EOverallQualityType>(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel)));
		}
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
	UpdateGraphicsSelectionIndicators();
	if (mIsApplyingValueModel == false)
	{
		OnQualityRequested.Broadcast(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel));
		// 전용 수신자가 생기기 전까지의 기본 적용: 품질 단계를 목표 렌더 해상도(720p)로 바꿔 프로필에 반영한다(FPS와 동일 패턴).
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetOverallQuality(StaticCast<EOverallQualityType>(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel)));
		}
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
	UpdateGraphicsSelectionIndicators();
	if (mIsApplyingValueModel == false)
	{
		OnQualityRequested.Broadcast(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel));
		// 전용 수신자가 생기기 전까지의 기본 적용: 품질 단계를 목표 렌더 해상도(1080p)로 바꿔 프로필에 반영한다(FPS와 동일 패턴).
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetOverallQuality(StaticCast<EOverallQualityType>(RDSettingsPanel::ToQualityRequestValue(mValueModel.mQualityLevel)));
		}
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
		// 전용 오디오 정책 시스템이 생기기 전까지 프로필 서브시스템으로 기본 적용한다(Master와 동일 패턴).
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetBGMVolume(mValueModel.mBgmVolume);
		}
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
		// 전용 오디오 정책 시스템이 생기기 전까지 프로필 서브시스템으로 기본 적용한다(Master와 동일 패턴).
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetSFXVolume(mValueModel.mSfxVolume);
		}
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
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetUIVolume(mValueModel.mUiVolume);
		}
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

/**
 * @brief 전체(마스터) 볼륨 변경을 이벤트로 올리고 프로필에 기본 적용한다.
 *
 * @details
 * 볼륨 이벤트들은 아직 전용 수신 시스템이 없다. 소리가 실제로 반응해야 설정 화면이 의미가 있으므로,
 * 프로필 서브시스템 공개 API(SetVolume)로 기본 적용을 함께 수행한다.
 * 전용 수신자(오디오 정책 시스템)가 생기면 이 직접 호출은 제거한다.
 */
void USettingsPanelWidget::HandleMasterVolumeChanged(float Value)
{
	mValueModel.mMasterVolume = RDSettingsPanel::NormalizeVolumeValue(Value);
	if (mIsApplyingValueModel == false)
	{
		OnMasterVolumeChanged.Broadcast(mValueModel.mMasterVolume);
		// 전용 오디오 정책 시스템이 생기기 전까지 프로필 서브시스템으로 기본 적용한다(Master와 동일 패턴).
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetMasterVolume(mValueModel.mMasterVolume);
		}
	}
}

/** @brief FPS 30 선택. 수신 시스템이 아직 없어 값 보관 + 이벤트 발신까지만 한다. */
void USettingsPanelWidget::HandleFpsThirtyButtonClicked()
{
	mValueModel.mFpsLimit = 30;
	UpdateGraphicsSelectionIndicators();
	if (mIsApplyingValueModel == false)
	{
		OnFpsLimitRequested.Broadcast(mValueModel.mFpsLimit);
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetFpsLimit(mValueModel.mFpsLimit);
		}
	}
}

/** @brief FPS 60 선택. 수신 시스템이 아직 없어 값 보관 + 이벤트 발신까지만 한다. */
void USettingsPanelWidget::HandleFpsSixtyButtonClicked()
{
	mValueModel.mFpsLimit = 60;
	UpdateGraphicsSelectionIndicators();
	if (mIsApplyingValueModel == false)
	{
		OnFpsLimitRequested.Broadcast(mValueModel.mFpsLimit);
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetFpsLimit(mValueModel.mFpsLimit);
		}
	}
}

/**
 * @brief 한국어 선택을 이벤트로 올리고 로컬라이제이션에 기본 적용한다.
 *
 * @details
 * 언어 저장/적용 경로(UOptionPersistData::SetLanguage -> SetCurrentCulture)는 완비되어 있고
 * 호출자만 없었다. 프로필 서브시스템 공개 API로 기본 적용한다(전용 수신자가 생기면 제거).
 */
void USettingsPanelWidget::HandleLanguageKoreanButtonClicked()
{
	mValueModel.mUseKoreanLanguage = true;
	if (mIsApplyingValueModel == false)
	{
		OnLanguageRequested.Broadcast(StaticCast<int32>(ELanguageType::KOREAN));
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetLanguage(ELanguageType::KOREAN);
		}
		SyncText();
	}
}

/** @brief English 선택을 이벤트로 올리고 로컬라이제이션에 기본 적용한다. */
void USettingsPanelWidget::HandleLanguageEnglishButtonClicked()
{
	mValueModel.mUseKoreanLanguage = false;
	if (mIsApplyingValueModel == false)
	{
		OnLanguageRequested.Broadcast(StaticCast<int32>(ELanguageType::ENGLISH));
		if (ARDGameModeBase* GameModeBase = GetWorld()->GetAuthGameMode<ARDGameModeBase>())
		{
			GameModeBase->SetLanguage(ELanguageType::ENGLISH);
		}
		SyncText();
	}
}

/** @brief 전투 이펙트 표시 체크 상태를 이벤트로 올린다(수신 VFX 시스템 미구현 - 값 전달만). */
void USettingsPanelWidget::HandleEffectsChanged(bool bChecked)
{
	mValueModel.mEffectsEnabled = bChecked;
	if (mIsApplyingValueModel == false)
	{
		URunPersistData::mFixedTestSeed = bChecked;
		OnEffectsChanged.Broadcast(mValueModel.mEffectsEnabled);
	}
}
