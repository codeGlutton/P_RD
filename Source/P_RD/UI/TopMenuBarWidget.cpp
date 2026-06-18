#include "UI/TopMenuBarWidget.h"

#include "Components/Button.h"
#include "UI/ViewportZOrderType.h"

/** @brief 탑바가 다른 팝업 위에서 보이도록 ZOrder를 설정한다. */
// 월드맵/설정 패널도 팝업 ZOrder를 사용하므로, 탑바는 PopUp보다 한 단계 위에 둔다.
// 대신 입력은 ApplyInputPassThrough()에서 버튼만 받게 처리해, 위에 보이더라도 아래 팝업 조작을 막지 않는다.
UTopMenuBarWidget::UTopMenuBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp) + 1;
}

/** @brief 위젯 구성 직후 버튼/전투 이벤트를 연결하고 입력 통과 상태를 적용한다. */
// 탑바는 직접 화면을 소유하지 않고, 버튼 입력과 전투 종료 신호를 받아 공용 월드 위젯을 여는 허브 역할을 한다.
// 왜 Construct에서 이벤트를 묶는가:
// WBP 버튼과 전투 서브시스템이 모두 준비된 뒤에만 구독할 수 있다. 여기서 한 번 연결해 두면 방마다 별도 버튼 연결을 만들지 않아도 된다.
void UTopMenuBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();
	SyncDefaultText();
	BindButtonEvents();
	EnsureRuntimeTopBarHitAreas();
	BindCombatEvents();
	ApplyInputPassThrough();
}

/** @brief 탑바가 사라질 때 외부 위젯과 서브시스템에 남긴 구독을 정리한다. */
void UTopMenuBarWidget::NativeDestruct()
{
	UnbindCombatEvents();
	UnbindPanelEvents();
	UnbindButtonEvents();
	if (mRuntimeDiceHitButton != nullptr)
	{
		mRuntimeDiceHitButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleDiceButtonClicked);
		mRuntimeDiceHitButton->RemoveFromParent();
	}
	if (mRuntimeSkillHitButton != nullptr)
	{
		mRuntimeSkillHitButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleSkillButtonClicked);
		mRuntimeSkillHitButton->RemoveFromParent();
	}

	Super::NativeDestruct();
}

/** @brief OpenUI()로 다시 열릴 때 현재 방 요약과 입력 통과 상태를 최신화한다. */
void UTopMenuBarWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	RefreshRoomInfo();
	EnsureRuntimeTopBarHitAreas();
	ApplyInputPassThrough();
}

/** @brief 연출용 표시 상태로 전환해 모든 입력을 통과시킨다. */
void UTopMenuBarWidget::ApplyDisplayOnly()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
