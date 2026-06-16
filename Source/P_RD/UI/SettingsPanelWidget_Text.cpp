#include "UI/SettingsPanelWidget.h"

#include "Components/TextBlock.h"

/**
 * @brief 외부 처리 흐름이 결정한 상태 문구를 표시한다.
 *
 * @details
 * 저장 중, 저장 완료, 저장 실패처럼 실제 결과를 아는 쪽은 설정 패널이 아니라 이벤트 수신자다.
 * 따라서 이 함수는 들어온 문구를 그대로 보여주는 표시 함수이며, 빈 문구는 상태 영역을 접어 평상시 레이아웃을 깔끔하게 유지한다.
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
 * @brief WBP에 배치된 텍스트 위젯에 기본 문구를 채운다.
 *
 * @details
 * 최종 설정 저장 로직이 붙기 전에도 화면이 비어 보이지 않게 하기 위한 기본 표시값이다.
 * 각 TextBlock이 Optional인 이유는 WBP 구조가 단계적으로 정리되는 중에도 C++ 위젯 생성이 실패하지 않게 하기 위해서다.
 * 연결된 위젯이 있을 때만 텍스트를 채워, 같은 C++ 클래스를 여러 WBP 변형에서 안전하게 쓸 수 있다.
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
 * @brief WBP_SettingsPanel의 최소 필수 바인딩 상태를 로그로 확인한다.
 *
 * @details
 * 이 브랜치에서는 WBP를 한 번에 완성된 설정 시스템으로 고정하지 않고, 공통 패널로 재사용할 수 있는 뼈대를 먼저 연결한다.
 * 그래서 대부분의 BindWidget은 Optional로 두지만, BackButton처럼 패널을 닫는 최소 동선은 빠지면 사용자가 갇히므로 경고를 남긴다.
 *
 * 왜 check가 아니라 Warning인가:
 * UI 이관 중에는 일부 디자이너 위젯이 아직 없거나 이름이 바뀌는 중일 수 있다.
 * 패키징/플레이 자체를 중단하기보다는 로그로 빠진 바인딩을 드러내고, 가능한 입력만 동작하게 하는 편이 단계별 이관에 맞다.
 */
void USettingsPanelWidget::ValidateDesignerBindings() const
{
	if (BackButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("SettingsPanelWidget: BackButton is not connected."));
	}
}
