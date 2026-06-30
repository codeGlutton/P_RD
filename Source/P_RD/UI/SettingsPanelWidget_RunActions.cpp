#include "UI/SettingsPanelWidget.h"

#include "Components/Button.h"
#include "Components/Widget.h"

/**
 * @brief 타이틀/인게임 모드를 바꾸고 모드별 영역 표시를 다시 적용한다.
 *
 * @details
 * 모드 변경은 이 위젯 안의 UI 표시 상태만 바꾼다.
 * 타이틀 모드가 됐다고 타이틀로 이동하거나, 인게임 모드가 됐다고 런 상태를 조회하지 않는다.
 * 그런 흐름은 이 패널을 여는 TitleMenuWidget 또는 GameMode 쪽의 책임이다.
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
