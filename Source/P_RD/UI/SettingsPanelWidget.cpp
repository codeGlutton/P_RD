#include "UI/SettingsPanelWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
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
