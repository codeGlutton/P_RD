#include "UI/SettingsPanelWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/ViewportZOrderType.h"

/**
 * @brief 설정 패널을 팝업 레이어에 표시되도록 초기화한다.
 */
USettingsPanelWidget::USettingsPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = static_cast<int32>(EViewportZOrderType::PopUp);
}

/**
 * @brief WBP 입력을 이벤트 발신용 핸들러에 연결하고 초기 표시 상태를 동기화한다.
 *
 * @details
 * 이 위젯은 설정 적용이나 런 종료를 직접 수행하지 않는다.
 * 버튼/슬라이더 입력을 외부 이벤트로 올리기 위해 Construct 시점에 WBP 컨트롤만 연결한다.
 */
void USettingsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();

	if (BackButton != nullptr)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleBackButtonClicked);
	}
	if (SaveAndExitButton != nullptr)
	{
		SaveAndExitButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleSaveAndExitButtonClicked);
	}
	if (AbandonRunButton != nullptr)
	{
		AbandonRunButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleAbandonRunButtonClicked);
	}
	if (ConfirmAbandonButton != nullptr)
	{
		ConfirmAbandonButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleConfirmAbandonButtonClicked);
	}
	if (CancelAbandonButton != nullptr)
	{
		CancelAbandonButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleCancelAbandonButtonClicked);
	}
	if (ResetButton != nullptr)
	{
		ResetButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleResetButtonClicked);
	}
	if (LowQualityButton != nullptr)
	{
		LowQualityButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleLowQualityButtonClicked);
	}
	if (MediumQualityButton != nullptr)
	{
		MediumQualityButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleMediumQualityButtonClicked);
	}
	if (HighQualityButton != nullptr)
	{
		HighQualityButton->OnClicked.AddUniqueDynamic(this, &USettingsPanelWidget::HandleHighQualityButtonClicked);
	}
	if (BgmVolumeSlider != nullptr)
	{
		BgmVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleBgmVolumeChanged);
	}
	if (SfxVolumeSlider != nullptr)
	{
		SfxVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleSfxVolumeChanged);
	}
	if (UiVolumeSlider != nullptr)
	{
		UiVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleUiVolumeChanged);
	}
	if (ScreenShakeCheckBox != nullptr)
	{
		ScreenShakeCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleScreenShakeChanged);
	}
	if (VibrationCheckBox != nullptr)
	{
		VibrationCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &USettingsPanelWidget::HandleVibrationChanged);
	}

	SyncText();
	ApplyModeVisibility();
	HideDeprecatedLanguageControls();
	HideAbandonConfirm();
}

/**
 * @brief NativeConstruct()에서 연결한 WBP 입력 이벤트를 해제한다.
 */
void USettingsPanelWidget::NativeDestruct()
{
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleBackButtonClicked);
	}
	if (SaveAndExitButton != nullptr)
	{
		SaveAndExitButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleSaveAndExitButtonClicked);
	}
	if (AbandonRunButton != nullptr)
	{
		AbandonRunButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleAbandonRunButtonClicked);
	}
	if (ConfirmAbandonButton != nullptr)
	{
		ConfirmAbandonButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleConfirmAbandonButtonClicked);
	}
	if (CancelAbandonButton != nullptr)
	{
		CancelAbandonButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleCancelAbandonButtonClicked);
	}
	if (ResetButton != nullptr)
	{
		ResetButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleResetButtonClicked);
	}
	if (LowQualityButton != nullptr)
	{
		LowQualityButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleLowQualityButtonClicked);
	}
	if (MediumQualityButton != nullptr)
	{
		MediumQualityButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleMediumQualityButtonClicked);
	}
	if (HighQualityButton != nullptr)
	{
		HighQualityButton->OnClicked.RemoveDynamic(this, &USettingsPanelWidget::HandleHighQualityButtonClicked);
	}
	if (BgmVolumeSlider != nullptr)
	{
		BgmVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleBgmVolumeChanged);
	}
	if (SfxVolumeSlider != nullptr)
	{
		SfxVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleSfxVolumeChanged);
	}
	if (UiVolumeSlider != nullptr)
	{
		UiVolumeSlider->OnValueChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleUiVolumeChanged);
	}
	if (ScreenShakeCheckBox != nullptr)
	{
		ScreenShakeCheckBox->OnCheckStateChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleScreenShakeChanged);
	}
	if (VibrationCheckBox != nullptr)
	{
		VibrationCheckBox->OnCheckStateChanged.RemoveDynamic(this, &USettingsPanelWidget::HandleVibrationChanged);
	}

	Super::NativeDestruct();
}

/**
 * @brief 타이틀/인게임 모드를 바꾸고 모드별 영역 표시를 다시 적용한다.
 */
void USettingsPanelWidget::SetPanelMode(ESettingsPanelMode NewPanelMode)
{
	mPanelMode = NewPanelMode;
	ApplyModeVisibility();
}

/**
 * @brief 현재 패널 모드를 반환한다.
 */
ESettingsPanelMode USettingsPanelWidget::GetPanelMode() const
{
	return mPanelMode;
}

/**
 * @brief 외부 런 상태에 맞춰 런 관련 버튼 활성 상태를 갱신한다.
 *
 * @details
 * 저장 가능 여부와 포기 가능 여부는 GameMode/저장 시스템이 판단한다.
 * 패널은 전달받은 결과만 표시해 UI와 런 정책을 분리한다.
 */
void USettingsPanelWidget::RefreshPanelState(bool bCanSaveRun, bool bCanAbandonRun)
{
	SetButtonEnabled(SaveAndExitButton, bCanSaveRun);
	SetButtonEnabled(AbandonRunButton, bCanAbandonRun);
	ApplyModeVisibility();
}

/**
 * @brief 외부 처리 흐름이 결정한 상태 문구를 표시한다.
 */
void USettingsPanelWidget::SetStatusText(const FText& Text) const
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(Text);
		StatusText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

/**
 * @brief 저장/포기 같은 런 액션 버튼의 중복 입력을 제어한다.
 */
void USettingsPanelWidget::SetRunActionsEnabled(bool bEnabled) const
{
	SetButtonEnabled(SaveAndExitButton, bEnabled);
	SetButtonEnabled(AbandonRunButton, bEnabled);
}

/**
 * @brief 런 포기 확정 패널을 표시한다.
 */
void USettingsPanelWidget::ShowAbandonConfirm() const
{
	if (AbandonConfirmPanel != nullptr)
	{
		AbandonConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

/**
 * @brief 런 포기 확정 패널을 숨긴다.
 */
void USettingsPanelWidget::HideAbandonConfirm() const
{
	if (AbandonConfirmPanel != nullptr)
	{
		AbandonConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

/**
 * @brief WBP에 배치된 텍스트 위젯에 기본 문구를 채운다.
 *
 * @details
 * 최종 설정 저장 로직이 붙기 전에도 화면이 비어 보이지 않게 하기 위한 기본 표시값이다.
 */
void USettingsPanelWidget::SyncText() const
{
	if (SettingsTitleText != nullptr)
	{
		SettingsTitleText->SetText(NSLOCTEXT("SettingsPanelWidget", "TitleText", "Settings"));
	}
	if (BackButtonText != nullptr)
	{
		BackButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "BackText", "Back"));
	}
	if (SaveAndExitButtonText != nullptr)
	{
		SaveAndExitButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "SaveAndExitText", "Save and Exit"));
	}
	if (AbandonRunButtonText != nullptr)
	{
		AbandonRunButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "AbandonRunText", "Abandon Run"));
	}
	if (ResetButtonText != nullptr)
	{
		ResetButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "ResetText", "Reset"));
	}
	if (AudioSectionHeader != nullptr)
	{
		AudioSectionHeader->SetText(NSLOCTEXT("SettingsPanelWidget", "AudioSectionText", "Audio"));
	}
	if (MasterVolumeRow_Label != nullptr)
	{
		MasterVolumeRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "MasterVolumeText", "Master"));
	}
	if (BGMVolumeRow_Label != nullptr)
	{
		BGMVolumeRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "BGMVolumeText", "BGM"));
	}
	if (SFXVolumeRow_Label != nullptr)
	{
		SFXVolumeRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "SFXVolumeText", "SFX"));
	}
	if (UIVolumeRow_Label != nullptr)
	{
		UIVolumeRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "UIVolumeText", "UI"));
	}
	if (DisplaySectionHeader != nullptr)
	{
		DisplaySectionHeader->SetText(NSLOCTEXT("SettingsPanelWidget", "DisplaySectionText", "Display"));
	}
	if (BrightnessRow_Label != nullptr)
	{
		BrightnessRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "BrightnessText", "Brightness"));
	}
	if (ScreenShakeRow_Label != nullptr)
	{
		ScreenShakeRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "ScreenShakeText", "Screen Shake"));
	}
	if (VibrationRow_Label != nullptr)
	{
		VibrationRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "VibrationText", "Vibration"));
	}
	if (QualityRow_Label != nullptr)
	{
		QualityRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "QualityText", "Quality"));
	}
	if (GameplaySectionHeader != nullptr)
	{
		GameplaySectionHeader->SetText(NSLOCTEXT("SettingsPanelWidget", "GameplaySectionText", "Gameplay"));
	}
	if (FastModeRow_Label != nullptr)
	{
		FastModeRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "FastModeText", "Fast Mode"));
	}
	if (SkipAnimationRow_Label != nullptr)
	{
		SkipAnimationRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "SkipAnimationText", "Skip Animation"));
	}
	if (AutoEndTurnRow_Label != nullptr)
	{
		AutoEndTurnRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "AutoEndTurnText", "Auto End Turn"));
	}
	if (InfoSectionHeader != nullptr)
	{
		InfoSectionHeader->SetText(NSLOCTEXT("SettingsPanelWidget", "InfoSectionText", "Info"));
	}
	if (CreditsRow_Label != nullptr)
	{
		CreditsRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "CreditsText", "Credits"));
	}
	if (LicenseRow_Label != nullptr)
	{
		LicenseRow_Label->SetText(NSLOCTEXT("SettingsPanelWidget", "LicenseText", "License"));
	}
	if (CreditsOpenButtonText != nullptr)
	{
		CreditsOpenButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "CreditsOpenText", "Open"));
	}
	if (LicenseOpenButtonText != nullptr)
	{
		LicenseOpenButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "LicenseOpenText", "Open"));
	}
	if (LowQualityButtonText != nullptr)
	{
		LowQualityButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "LowQualityText", "LOW"));
	}
	if (MediumQualityButtonText != nullptr)
	{
		MediumQualityButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "MediumQualityText", "MID"));
	}
	if (HighQualityButtonText != nullptr)
	{
		HighQualityButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "HighQualityText", "HIGH"));
	}
	if (AbandonConfirmTitleText != nullptr)
	{
		AbandonConfirmTitleText->SetText(NSLOCTEXT("SettingsPanelWidget", "AbandonConfirmTitle", "Abandon this run?"));
	}
	if (AbandonConfirmBodyText != nullptr)
	{
		AbandonConfirmBodyText->SetText(NSLOCTEXT("SettingsPanelWidget", "AbandonConfirmBody", "Abandoning resets the current run and returns to the title."));
	}
	if (ConfirmAbandonButtonText != nullptr)
	{
		ConfirmAbandonButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "ConfirmAbandonText", "Abandon"));
	}
	if (CancelAbandonButtonText != nullptr)
	{
		CancelAbandonButtonText->SetText(NSLOCTEXT("SettingsPanelWidget", "CancelText", "Cancel"));
	}
}

/**
 * @brief 현재 브랜치에서 사용하지 않는 언어 설정 영역을 숨긴다.
 */
void USettingsPanelWidget::HideDeprecatedLanguageControls() const
{
	if (LanguageSectionHeader != nullptr)
	{
		LanguageSectionHeader->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (LanguageRow != nullptr)
	{
		LanguageRow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

/**
 * @brief 패널 모드에 따라 런 액션 영역 표시를 전환한다.
 */
void USettingsPanelWidget::ApplyModeVisibility() const
{
	if (RunActionsPanel != nullptr)
	{
		RunActionsPanel->SetVisibility(mPanelMode == ESettingsPanelMode::InGame ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

/**
 * @brief WBP_SettingsPanel의 최소 필수 바인딩 상태를 로그로 확인한다.
 */
void USettingsPanelWidget::ValidateDesignerBindings() const
{
	if (BackButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("SettingsPanelWidget: BackButton is not connected."));
	}
}

/**
 * @brief 버튼 포인터가 있을 때만 활성 상태를 바꾼다.
 */
void USettingsPanelWidget::SetButtonEnabled(UButton* Button, bool bEnabled) const
{
	if (Button != nullptr)
	{
		Button->SetIsEnabled(bEnabled);
	}
}

/**
 * @brief Back 버튼 입력을 외부 복귀 요청 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleBackButtonClicked()
{
	OnBackRequested.Broadcast();
}

/**
 * @brief 저장 후 나가기 버튼 입력을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleSaveAndExitButtonClicked()
{
	OnSaveAndExitRequested.Broadcast();
}

/**
 * @brief 런 포기 버튼 입력을 즉시 실행하지 않고 확인 패널 표시로 연결한다.
 */
void USettingsPanelWidget::HandleAbandonRunButtonClicked()
{
	ShowAbandonConfirm();
}

/**
 * @brief 런 포기 확정 입력을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleConfirmAbandonButtonClicked()
{
	OnAbandonRunConfirmed.Broadcast();
}

/**
 * @brief 런 포기 확인 취소 입력을 패널 닫기로 처리한다.
 */
void USettingsPanelWidget::HandleCancelAbandonButtonClicked()
{
	HideAbandonConfirm();
}

/**
 * @brief 설정 초기화 입력을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleResetButtonClicked()
{
	OnResetRequested.Broadcast();
}

/**
 * @brief 낮음 품질 선택 요청을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleLowQualityButtonClicked()
{
	OnQualityRequested.Broadcast(0);
}

/**
 * @brief 중간 품질 선택 요청을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleMediumQualityButtonClicked()
{
	OnQualityRequested.Broadcast(1);
}

/**
 * @brief 높음 품질 선택 요청을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleHighQualityButtonClicked()
{
	OnQualityRequested.Broadcast(2);
}

/**
 * @brief BGM 볼륨 변경 값을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleBgmVolumeChanged(float Value)
{
	OnBgmVolumeChanged.Broadcast(Value);
}

/**
 * @brief 효과음 볼륨 변경 값을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleSfxVolumeChanged(float Value)
{
	OnSfxVolumeChanged.Broadcast(Value);
}

/**
 * @brief UI 볼륨 변경 값을 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleUiVolumeChanged(float Value)
{
	OnUiVolumeChanged.Broadcast(Value);
}

/**
 * @brief 화면 흔들림 체크 상태를 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleScreenShakeChanged(bool bChecked)
{
	OnScreenShakeChanged.Broadcast(bChecked);
}

/**
 * @brief 진동 체크 상태를 외부 이벤트로 전달한다.
 */
void USettingsPanelWidget::HandleVibrationChanged(bool bChecked)
{
	OnVibrationChanged.Broadcast(bChecked);
}
