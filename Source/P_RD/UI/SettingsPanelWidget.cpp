#include "UI/SettingsPanelWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/ViewportZOrderType.h"

/**
 * @brief 설정 패널을 팝업 레이어에 표시되도록 초기화한다.
 *
 * @details
 * 설정 패널은 타이틀 메뉴 위나 인게임 상단 메뉴 위에 뜨는 보조 화면이다.
 * 기본 ZOrder를 PopUp으로 두면 OpenUI()로 열릴 때 HUD/월드 위젯보다 앞에 배치되어 입력 우선순위가 자연스럽다.
 * 팝업 계층에 올라와야 Back/Save/Abandon 같은 버튼 입력이 뒤쪽 메뉴나 HUD에 섞이지 않는다.
 */
USettingsPanelWidget::USettingsPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
}

/**
 * @brief WBP 버튼/슬라이더를 이 위젯의 입력 처리 함수에 연결하고 처음 표시 상태를 맞춘다.
 *
 * @details
 * SettingsPanelWidget은 설정 화면의 입력만 받는다. 저장, 런 포기, 타이틀 이동 같은 실제 게임 처리는
 * 직접 하지 않고 OnSaveAndExitRequested, OnAbandonRunConfirmed 같은 이벤트로 바깥에 알려준다.
 *
 * 왜 이렇게 나누는가:
 * 같은 설정 패널을 타이틀과 인게임에서 함께 쓰기 때문이다. 패널 안에 GameMode/저장/전환 로직을 넣으면
 * 타이틀용 설정과 인게임용 설정이 서로의 흐름을 알아야 하므로 재사용하기 어렵다.
 *
 * 구성 순서:
 * 먼저 WBP 바인딩 누락을 로그로 확인하고, 버튼/슬라이더/체크박스 입력을 C++ 핸들러에 연결한다.
 * 그 뒤 기본 문구, 모드별 표시 상태, 런 포기 확인 패널의 초기 상태를 맞춘다.
 */
void USettingsPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();

	/* 화면 복귀와 런 액션 버튼은 클릭을 외부 요청 이벤트로만 변환한다. */

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

	/* 품질 버튼은 현재 패널의 임시 품질 번호를 외부 설정 정책으로 전달한다. */

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

	/* 값 입력 위젯은 입력 값을 그대로 이벤트로 올리고, 실제 적용/저장은 외부 시스템에 맡긴다. */

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

	/* WBP가 가진 임시 표시 상태를 C++ 기본 표시 정책으로 정리한다. */

	SyncText();
	ApplyModeVisibility();
	HideAbandonConfirm();
}

/**
 * @brief NativeConstruct()에서 연결한 WBP 입력 이벤트를 해제한다.
 *
 * @details
 * UMG 위젯은 화면에서 빠졌다가 다시 붙거나, 에디터에서 재생을 반복하면서 Construct/Destruct를 여러 번 거칠 수 있다.
 * Construct에서 AddUniqueDynamic을 사용해 중복 연결을 줄이고, Destruct에서 RemoveDynamic으로 정리해 생명주기 끝에 입력 연결이 남지 않게 한다.
 *
 * 왜 모든 입력을 다시 풀어주는가:
 * 설정 패널은 OpenUI/CloseUI 흐름으로 여러 번 열릴 수 있다. 이전 인스턴스의 Delegate가 남아 있으면 버튼 한 번에
 * 같은 요청이 여러 번 Broadcast될 수 있으므로, Construct에서 연결한 항목은 Destruct에서 같은 목록으로 해제한다.
 */
void USettingsPanelWidget::NativeDestruct()
{
	/* NativeConstruct()에서 연결한 버튼 입력 Delegate를 해제한다. */

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

	/* 슬라이더/체크박스 값 변경 Delegate도 같은 생명주기에서 정리한다. */

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
 *
 * @details
 * 모드 변경은 이 위젯 안의 UI 표시 상태만 바꾼다.
 * 타이틀 모드가 됐다고 타이틀로 이동하거나, 인게임 모드가 됐다고 런 상태를 조회하지 않는다.
 * 그런 흐름은 이 패널을 여는 TitleMenuWidget 또는 TopMenuBarWidget 쪽의 책임이다.
 */
void USettingsPanelWidget::SetPanelMode(ESettingsPanelMode NewPanelMode)
{
	mPanelMode = NewPanelMode;
	ApplyModeVisibility();
}

/**
 * @brief 현재 패널 모드를 반환한다.
 *
 * @details
 * 외부 코드가 같은 SettingsPanelWidget 인스턴스를 받았을 때 현재 어떤 화면 정책으로 표시 중인지 확인할 수 있게 한다.
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
 *
 * 왜 bool만 받는가:
 * 패널은 "이 버튼을 지금 눌러도 되는가"만 알면 된다. 실제 저장 가능 조건을 직접 검사하면
 * 설정 UI가 런 데이터 구조와 저장 정책에 묶여 타이틀 화면에서 재사용하기 어려워진다.
 */
void USettingsPanelWidget::RefreshPanelState(bool bCanSaveRun, bool bCanAbandonRun)
{
	SetButtonEnabled(SaveAndExitButton, bCanSaveRun);
	SetButtonEnabled(AbandonRunButton, bCanAbandonRun);
	ApplyModeVisibility();
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
 * @brief 저장/포기 같은 런 액션 버튼의 중복 입력을 제어한다.
 *
 * @details
 * 런 저장이나 포기 처리 중에는 같은 요청이 두 번 들어오면 전환 상태와 저장 상태가 어긋날 수 있다.
 * 볼륨/품질 같은 일반 설정은 계속 조작 가능하게 두고, 런 흐름을 바꾸는 버튼만 잠그는 용도다.
 */
void USettingsPanelWidget::SetRunActionsEnabled(bool bEnabled) const
{
	SetButtonEnabled(SaveAndExitButton, bEnabled);
	SetButtonEnabled(AbandonRunButton, bEnabled);
}

/**
 * @brief 런 포기 확정 패널을 표시한다.
 *
 * @details
 * "런 포기" 버튼은 위험한 액션의 1차 진입점이므로 이 함수에서는 확인 UI만 연다.
 * 실제 포기 실행은 Confirm 버튼을 눌러 OnAbandonRunConfirmed가 Broadcast된 뒤 외부 흐름에서 처리한다.
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
 *
 * @details
 * 취소하거나 패널을 초기 상태로 되돌릴 때 확인 UI만 접는다.
 * 이 함수는 확인 패널의 가시성만 바꾸며, 런 데이터나 저장 상태에는 손대지 않는다.
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
 * @brief 패널 모드에 따라 런 액션 영역 표시를 전환한다.
 *
 * @details
 * InGame 모드에서는 저장 후 종료/런 포기 같은 런 액션을 보여주고, Title 모드에서는 숨긴다.
 *
 * 왜 표시만 전환하는가:
 * 버튼 자체를 별도 WBP로 분리하면 타이틀과 인게임 설정 화면이 금방 갈라진다.
 * 같은 패널 안에서 모드별 차이만 접으면 공통 설정 UI의 모양과 입력 연결을 한 곳에서 유지할 수 있다.
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

/**
 * @brief 버튼 포인터가 있을 때만 활성 상태를 바꾼다.
 *
 * @details
 * WBP 바인딩이 Optional이므로 버튼이 없는 WBP 변형에서도 호출자가 매번 nullptr 검사를 반복하지 않게 하는 작은 방어 함수다.
 * 이 함수가 없으면 RefreshPanelState(), SetRunActionsEnabled() 같은 외부 API가 WBP 내부 구성에 너무 민감해진다.
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
 *
 * @details
 * 이 함수는 화면을 닫지 않고 OnBackRequested만 Broadcast한다.
 * 타이틀에서는 타이틀 메뉴로 돌아가고, 인게임에서는 상단 메뉴 흐름으로 돌아가는 식으로 수신자마다 복귀 방식이 다르기 때문이다.
 */
void USettingsPanelWidget::HandleBackButtonClicked()
{
	OnBackRequested.Broadcast();
}

/**
 * @brief 저장 후 나가기 버튼 입력을 외부 이벤트로 전달한다.
 *
 * @details
 * 저장 가능 여부, 저장 실행, 저장 후 타이틀 이동은 모두 런 상태를 알고 있는 쪽에서 처리해야 한다.
 * 패널은 클릭이 발생했다는 사실만 OnSaveAndExitRequested로 알린다.
 */
void USettingsPanelWidget::HandleSaveAndExitButtonClicked()
{
	OnSaveAndExitRequested.Broadcast();
}

/**
 * @brief 런 포기 버튼 입력을 즉시 실행하지 않고 확인 패널 표시로 연결한다.
 *
 * @details
 * 런 포기는 현재 진행을 폐기하는 위험한 액션이다.
 * 첫 클릭에서는 ShowAbandonConfirm()만 호출해 사용자가 한 번 더 확인하도록 하고, 실제 포기 이벤트는 Confirm 버튼에서만 발생시킨다.
 */
void USettingsPanelWidget::HandleAbandonRunButtonClicked()
{
	ShowAbandonConfirm();
}

/**
 * @brief 런 포기 확정 입력을 외부 이벤트로 전달한다.
 *
 * @details
 * 이 시점은 확인 패널에서 사용자가 최종 승인한 뒤다.
 * 그래도 위젯이 직접 런을 폐기하지는 않고, OnAbandonRunConfirmed를 받은 인게임 흐름이 저장/전환 순서를 책임진다.
 */
void USettingsPanelWidget::HandleConfirmAbandonButtonClicked()
{
	OnAbandonRunConfirmed.Broadcast();
}

/**
 * @brief 런 포기 확인 취소 입력을 패널 닫기로 처리한다.
 *
 * @details
 * 취소는 순수 UI 액션이다. 외부 시스템에 알릴 필요 없이 확인 패널만 접고 기존 설정 패널 상태를 유지한다.
 */
void USettingsPanelWidget::HandleCancelAbandonButtonClicked()
{
	HideAbandonConfirm();
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
	OnQualityRequested.Broadcast(0);
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
	OnQualityRequested.Broadcast(1);
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
	OnQualityRequested.Broadcast(2);
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
	OnBgmVolumeChanged.Broadcast(Value);
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
	OnSfxVolumeChanged.Broadcast(Value);
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
	OnUiVolumeChanged.Broadcast(Value);
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
	OnScreenShakeChanged.Broadcast(bChecked);
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
	OnVibrationChanged.Broadcast(bChecked);
}
