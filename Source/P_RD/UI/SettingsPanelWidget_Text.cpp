#include "UI/SettingsPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ContentWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "SettingsPanelWidget_Text"

namespace
{
	/** @brief 버튼 등 컨테이너 위젯 아래에서 첫 TextBlock을 찾는다(SyncText의 탐색과 동일 규칙). */
	UTextBlock* FindFirstTextBlockIn(UWidget* Root)
	{
		if (Root == nullptr)
		{
			return nullptr;
		}
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Root))
		{
			return TextBlock;
		}
		if (UContentWidget* ContentWidget = Cast<UContentWidget>(Root))
		{
			if (UTextBlock* TextBlock = FindFirstTextBlockIn(ContentWidget->GetContent()))
			{
				return TextBlock;
			}
		}
		if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Root))
		{
			const int32 ChildCount = PanelWidget->GetChildrenCount();
			for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
			{
				if (UTextBlock* TextBlock = FindFirstTextBlockIn(PanelWidget->GetChildAt(ChildIndex)))
				{
					return TextBlock;
				}
			}
		}
		return nullptr;
	}
}

void USettingsPanelWidget::UpdateGraphicsSelectionIndicators() const
{
	// 패널 프레임 골드 톤과 맞춘 선택 색. WBP에 선택 전용 위젯이 생기면 이 함수는 그것으로 대체한다.
	const FSlateColor SelectedColor(FLinearColor(1.0f, 0.78f, 0.30f));
	const FSlateColor UnselectedColor(FLinearColor::White);

	const auto SetButtonLabelColor = [&SelectedColor, &UnselectedColor](UButton* Button, bool bSelected)
	{
		if (Button == nullptr)
		{
			return;
		}
		if (UTextBlock* Label = FindFirstTextBlockIn(Button))
		{
			Label->SetColorAndOpacity(bSelected ? SelectedColor : UnselectedColor);
		}
	};

	SetButtonLabelColor(LowQualityButton, mValueModel.mQualityLevel == ESettingsQualityLevel::Low);
	SetButtonLabelColor(MediumQualityButton, mValueModel.mQualityLevel == ESettingsQualityLevel::Medium);
	SetButtonLabelColor(HighQualityButton, mValueModel.mQualityLevel == ESettingsQualityLevel::High);
	SetButtonLabelColor(FpsThirtyButton, mValueModel.mFpsLimit == 30);
	SetButtonLabelColor(FpsSixtyButton, mValueModel.mFpsLimit == 60);
}

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
	const TFunction<UTextBlock*(UWidget*)> FindFirstTextBlock = [&FindFirstTextBlock](UWidget* Root) -> UTextBlock*
	{
		if (Root == nullptr)
		{
			return nullptr;
		}
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Root))
		{
			return TextBlock;
		}
		if (UContentWidget* ContentWidget = Cast<UContentWidget>(Root))
		{
			if (UTextBlock* TextBlock = FindFirstTextBlock(ContentWidget->GetContent()))
			{
				return TextBlock;
			}
		}
		if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Root))
		{
			const int32 ChildCount = PanelWidget->GetChildrenCount();
			for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
			{
				if (UTextBlock* TextBlock = FindFirstTextBlock(PanelWidget->GetChildAt(ChildIndex)))
				{
					return TextBlock;
				}
			}
		}
		return nullptr;
	};
	const auto SetNamedText = [this, &FindFirstTextBlock](const TCHAR* WidgetName, const FText& InText)
	{
		if (WidgetTree == nullptr)
		{
			return;
		}
		if (UTextBlock* TextBlock = FindFirstTextBlock(WidgetTree->FindWidget(WidgetName)))
		{
			TextBlock->SetText(InText);
		}
	};

	if (SettingsTitleText != nullptr)
	{
		SettingsTitleText->SetText(LOCTEXT("Settings", "Settings"));
	}
	SetNamedText(TEXT("SettingsTitleText"), LOCTEXT("Settings", "Settings"));
	SetNamedText(TEXT("Set_sec_graphics_text"), LOCTEXT("Graphics", "Graphics"));
	SetNamedText(TEXT("Set_sec_display_text"), LOCTEXT("Graphics", "Graphics"));
	SetNamedText(TEXT("Set_sec_audio_text"), LOCTEXT("Volume", "Volume"));
	SetNamedText(TEXT("Set_sec_volume_text"), LOCTEXT("Volume", "Volume"));
	SetNamedText(TEXT("Set_sec_gameplay_text"), LOCTEXT("Gameplay", "Gameplay"));
	SetNamedText(TEXT("Set_row_fps_label"), LOCTEXT("FPS", "FPS"));
	SetNamedText(TEXT("FpsThirtyButton"), LOCTEXT("30", "30"));
	SetNamedText(TEXT("FpsSixtyButton"), LOCTEXT("60", "60"));
	SetNamedText(TEXT("Set_row_quality_label"), LOCTEXT("Quality", "Quality"));
	SetNamedText(TEXT("QualityRow_Label"), LOCTEXT("Quality", "Quality"));
	SetNamedText(TEXT("QualityLowButton"), LOCTEXT("Low", "Low"));
	SetNamedText(TEXT("QualityMidButton"), LOCTEXT("Mid", "Mid"));
	SetNamedText(TEXT("QualityHighButton"), LOCTEXT("High", "High"));
	SetNamedText(TEXT("LowQualityButton"), LOCTEXT("Low", "Low"));
	SetNamedText(TEXT("MediumQualityButton"), LOCTEXT("Mid", "Mid"));
	SetNamedText(TEXT("HighQualityButton"), LOCTEXT("High", "High"));
	SetNamedText(TEXT("Set_row_screen_shake_label"), LOCTEXT("Screen Shake", "Screen Shake"));
	SetNamedText(TEXT("ScreenShakeRow_Label"), LOCTEXT("Screen Shake", "Screen Shake"));
	SetNamedText(TEXT("Set_row_effects_label"), LOCTEXT("Effects", "Effects"));
	SetNamedText(TEXT("Set_row_effects_text"), LOCTEXT("Effects", "Effects"));
	SetNamedText(TEXT("EffectsRow_Label"), LOCTEXT("Effects", "Effects"));
	SetNamedText(TEXT("Set_row_language_label"), LOCTEXT("Language", "Language"));
	SetNamedText(TEXT("LanguageRow_Label"), LOCTEXT("Language", "Language"));
	// 언어 이름은 번역하지 않는다 — 각 언어를 그 언어 자체 이름으로 보여줘야 사용자가 자기 언어를 알아본다.
	SetNamedText(TEXT("LanguageKoreanButton"), FText::FromString(TEXT("한국어")));
	SetNamedText(TEXT("LanguageEnglishButton"), FText::FromString(TEXT("English")));
	SetNamedText(TEXT("Set_row_master_label"), LOCTEXT("Master", "Master"));
	SetNamedText(TEXT("MasterVolumeRow_Label"), LOCTEXT("Master", "Master"));
	SetNamedText(TEXT("Set_row_bgm_label"), LOCTEXT("BGM", "BGM"));
	SetNamedText(TEXT("BGMVolumeRow_Label"), LOCTEXT("BGM", "BGM"));
	SetNamedText(TEXT("Set_row_sfx_label"), LOCTEXT("SFX", "SFX"));
	SetNamedText(TEXT("SFXVolumeRow_Label"), LOCTEXT("SFX", "SFX"));
	SetNamedText(TEXT("Set_row_ui_label"), LOCTEXT("UI", "UI"));
	SetNamedText(TEXT("UIVolumeRow_Label"), LOCTEXT("UI", "UI"));
	SetNamedText(TEXT("BackButton"), LOCTEXT("Back", "Back"));
	SetNamedText(TEXT("SaveAndExitButton"), LOCTEXT("Save and Exit", "Save and Exit"));
	SetNamedText(TEXT("AbandonRunButton"), LOCTEXT("Abandon Run", "Abandon Run"));
	SetNamedText(TEXT("ResetButton"), LOCTEXT("Reset", "Reset"));
	if (BackButtonText != nullptr)
	{
		BackButtonText->SetText(LOCTEXT("Back", "Back"));
	}
	if (SaveAndExitButtonText != nullptr)
	{
		SaveAndExitButtonText->SetText(LOCTEXT("Save and Exit", "Save and Exit"));
	}
	if (AbandonRunButtonText != nullptr)
	{
		AbandonRunButtonText->SetText(LOCTEXT("Abandon Run", "Abandon Run"));
	}
	if (ResetButtonText != nullptr)
	{
		ResetButtonText->SetText(LOCTEXT("Reset", "Reset"));
	}
	if (AudioSectionHeader != nullptr)
	{
		AudioSectionHeader->SetText(LOCTEXT("Audio", "Audio"));
	}
	if (MasterVolumeRow_Label != nullptr)
	{
		MasterVolumeRow_Label->SetText(LOCTEXT("Master", "Master"));
	}
	if (BGMVolumeRow_Label != nullptr)
	{
		BGMVolumeRow_Label->SetText(LOCTEXT("BGM", "BGM"));
	}
	if (SFXVolumeRow_Label != nullptr)
	{
		SFXVolumeRow_Label->SetText(LOCTEXT("SFX", "SFX"));
	}
	if (UIVolumeRow_Label != nullptr)
	{
		UIVolumeRow_Label->SetText(LOCTEXT("UI", "UI"));
	}
	if (DisplaySectionHeader != nullptr)
	{
		DisplaySectionHeader->SetText(LOCTEXT("Display", "Display"));
	}
	if (BrightnessRow_Label != nullptr)
	{
		BrightnessRow_Label->SetText(LOCTEXT("Brightness", "Brightness"));
	}
	if (ScreenShakeRow_Label != nullptr)
	{
		ScreenShakeRow_Label->SetText(LOCTEXT("Screen Shake", "Screen Shake"));
	}
	if (VibrationRow_Label != nullptr)
	{
		VibrationRow_Label->SetText(LOCTEXT("Vibration", "Vibration"));
	}
	if (QualityRow_Label != nullptr)
	{
		QualityRow_Label->SetText(LOCTEXT("Quality", "Quality"));
	}
	if (GameplaySectionHeader != nullptr)
	{
		GameplaySectionHeader->SetText(LOCTEXT("Gameplay", "Gameplay"));
	}
	if (FastModeRow_Label != nullptr)
	{
		FastModeRow_Label->SetText(LOCTEXT("Fast Mode", "Fast Mode"));
	}
	if (SkipAnimationRow_Label != nullptr)
	{
		SkipAnimationRow_Label->SetText(LOCTEXT("Skip Animation", "Skip Animation"));
	}
	if (AutoEndTurnRow_Label != nullptr)
	{
		AutoEndTurnRow_Label->SetText(LOCTEXT("Auto End Turn", "Auto End Turn"));
	}
	if (InfoSectionHeader != nullptr)
	{
		InfoSectionHeader->SetText(LOCTEXT("Info", "Info"));
	}
	if (CreditsRow_Label != nullptr)
	{
		CreditsRow_Label->SetText(LOCTEXT("Credits", "Credits"));
	}
	if (LicenseRow_Label != nullptr)
	{
		LicenseRow_Label->SetText(LOCTEXT("License", "License"));
	}
	if (CreditsOpenButtonText != nullptr)
	{
		CreditsOpenButtonText->SetText(LOCTEXT("Open", "Open"));
	}
	if (LicenseOpenButtonText != nullptr)
	{
		LicenseOpenButtonText->SetText(LOCTEXT("Open", "Open"));
	}
	if (LowQualityButtonText != nullptr)
	{
		LowQualityButtonText->SetText(LOCTEXT("LOW", "LOW"));
	}
	if (MediumQualityButtonText != nullptr)
	{
		MediumQualityButtonText->SetText(LOCTEXT("MID", "MID"));
	}
	if (HighQualityButtonText != nullptr)
	{
		HighQualityButtonText->SetText(LOCTEXT("HIGH", "HIGH"));
	}
	if (AbandonConfirmTitleText != nullptr)
	{
		AbandonConfirmTitleText->SetText(LOCTEXT("Abandon this run?", "Abandon this run?"));
	}
	if (AbandonConfirmBodyText != nullptr)
	{
		AbandonConfirmBodyText->SetText(LOCTEXT("Abandoning resets the current run and returns to the title.", "Abandoning resets the current run and returns to the title."));
	}
	if (ConfirmAbandonButtonText != nullptr)
	{
		ConfirmAbandonButtonText->SetText(LOCTEXT("Abandon", "Abandon"));
	}
	if (CancelAbandonButtonText != nullptr)
	{
		CancelAbandonButtonText->SetText(LOCTEXT("Cancel", "Cancel"));
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

#undef LOCTEXT_NAMESPACE
