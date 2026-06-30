#include "UI/SkillPanelWidget.h"

#include "Components/Button.h"
#include "UI/SkillPanelWidgetPrivate.h"

/** @brief 스킬 패널 기본 객체를 생성한다. */
// 현재 스킬 패널은 별도 게임 로직 없이 UCarouselPanelWidget의 카드 선택/닫기 동작을 그대로 사용한다.
// 실제 스킬 데이터와 사용 규칙이 붙기 전까지는 WBP 표시와 인게임 연결 검증용 패널로 둔다.
USkillPanelWidget::USkillPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USkillPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureSkillSelectButtons();
	EnsureSkillDetailWidgets();
}

void USkillPanelWidget::NativeDestruct()
{
	UnbindSkillSelectButtons();

	if (mSkillDetailBackButton != nullptr)
	{
		mSkillDetailBackButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillDetailBackButtonClicked);
	}
	if (mSkillDetailPreviousButton != nullptr)
	{
		mSkillDetailPreviousButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillDetailPreviousButtonClicked);
	}
	if (mSkillDetailNextButton != nullptr)
	{
		mSkillDetailNextButton->OnClicked.RemoveDynamic(this, &USkillPanelWidget::HandleSkillDetailNextButtonClicked);
	}

	Super::NativeDestruct();
}

void USkillPanelWidget::ApplyOpenUI()
{
	mShowingSkillDetail = false;
	mDetailSkillIndex = INDEX_NONE;
	mTrackingSkillDetailSwipe = false;
	mSkillDetailSwipeStartPosition = FVector2D::ZeroVector;

	Super::ApplyOpenUI();
	EnsureSkillSelectButtons();
	EnsureSkillDetailWidgets();
	RefreshSkillDetailWidgets();
	RefreshSkillSelectButtons();
}

int32 USkillPanelWidget::GetCarouselItemDisplayCount() const
{
	return RDSkillPanel::ItemCount;
}

void USkillPanelWidget::HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
	ShowSkillDetail(NewIndex);
}
