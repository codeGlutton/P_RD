#include "UI/CarouselPanelWidget.h"

#include "Components/Widget.h"
#include "UI/ViewportZOrderType.h"

/**
 * @brief 일반 팝업보다 살짝 위에 뜨는 캐러셀 패널 기본 ZOrder를 설정한다.
 *
 * @details
 * DicePanel/SkillPanel은 인게임 버튼으로 열리는 플로팅 패널이다.
 * 전투 HUD 위에서 조작되어야 하므로 일반 팝업보다 조금 높은 계층에 둔다.
 */
UCarouselPanelWidget::UCarouselPanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp) + 2;
}

/**
 * @brief WBP에 배치된 카드/버튼을 찾아 캐시하고 초기 목록 배치를 적용한다.
 *
 * @details
 * Construct 시점에는 WidgetTree가 완성되어 있어 이름 기반 검색이 가능하다.
 * 처음 열렸을 때는 원형 캐러셀이 아니라 가로 목록 상태로 보여주고,
 * 사용자가 하나를 선택하면 캐러셀 상태로 전환한다.
 */
void UCarouselPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheCarouselWidgets();
	BindCarouselEvents();
	ApplyCarouselLayout();
}

/**
 * @brief Construct에서 연결한 버튼 이벤트를 해제한다.
 *
 * @details
 * 팝업 위젯은 열고 닫히는 과정에서 다시 Construct될 수 있다.
 * 이벤트가 남아 있으면 한 번 클릭에 여러 선택 처리가 들어갈 수 있어 Destruct에서 정리한다.
 */
void UCarouselPanelWidget::NativeDestruct()
{
	UnbindCarouselEvents();

	Super::NativeDestruct();
}

/**
 * @brief OpenUI()로 패널이 열릴 때 선택/입력 상태를 초기화하고 목록 배치를 다시 적용한다.
 *
 * @details
 * 이전에 열었을 때 선택했던 카드나 누른 위치가 남아 있으면 새로 연 패널이 이미 선택된 상태처럼 보인다.
 * 패널을 열 때마다 "아직 카드 선택 전" 상태로 돌려, DICE/SKILL 모두 같은 시작 화면을 갖게 한다.
 */
void UCarouselPanelWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	ResetCarouselState();
	ApplyCarouselLayout();
}

/**
 * @brief 사용자가 카드 하나를 선택해 원형 캐러셀 상태로 들어갔는지 반환한다.
 */
bool UCarouselPanelWidget::IsCarouselActivated() const
{
	return mCarouselActivated;
}

/**
 * @brief 현재 선택된 카드 인덱스를 반환한다.
 */
int32 UCarouselPanelWidget::GetSelectedCarouselIndex() const
{
	return mSelectedCarouselIndex;
}

/**
 * @brief 현재 선택된 카드 위젯을 반환한다.
 */
UWidget* UCarouselPanelWidget::GetSelectedCarouselItem() const
{
	return mCarouselItems.IsValidIndex(mSelectedCarouselIndex) ? mCarouselItems[mSelectedCarouselIndex].Get() : nullptr;
}

/**
 * @brief 포인터 위치가 현재 선택된 카드 위에 있는지 확인한다.
 *
 * @details
 * DicePanel은 선택된 카드 위에서만 회전 드래그를 시작해야 한다.
 * 캐러셀 공통 베이스가 Geometry 판정을 제공해 파생 패널이 카드 배열 구조를 직접 만지지 않게 한다.
 */
bool UCarouselPanelWidget::IsPointerOverSelectedCarouselItem(const FVector2D& ScreenPosition) const
{
	const UWidget* SelectedItem = GetSelectedCarouselItem();
	return SelectedItem != nullptr && SelectedItem->GetCachedGeometry().IsUnderLocation(ScreenPosition);
}

/**
 * @brief 현재 패널에서 실제로 표시할 카드 개수를 반환한다.
 */
int32 UCarouselPanelWidget::GetCarouselItemDisplayCount() const
{
	return INDEX_NONE;
}

/**
 * @brief 카드별 추가 회전 각도를 반환한다.
 *
 * @details
 * 기본 캐러셀은 회전을 주지 않는다.
 * DicePanel은 선택된 카드만 드래그 각도로 회전시키기 위해 이 함수를 override한다.
 */
float UCarouselPanelWidget::GetCarouselItemAngle(int32 ItemIndex) const
{
	return 0.0f;
}

/**
 * @brief 선택 카드가 바뀐 뒤 파생 패널이 필요한 상태를 정리할 수 있는 hook이다.
 */
void UCarouselPanelWidget::HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
}

int32 UCarouselPanelWidget::GetClampedCarouselItemDisplayCount() const
{
	const int32 DisplayCount = GetCarouselItemDisplayCount();
	if (DisplayCount == INDEX_NONE)
	{
		return mCarouselItems.Num();
	}

	return FMath::Clamp(DisplayCount, 0, mCarouselItems.Num());
}

void UCarouselPanelWidget::SelectCarouselItem(int32 ItemIndex)
{
	/*
	 * 첫 선택 전에는 패널이 가로 목록처럼 보인다.
	 * 사용자가 카드를 선택하면 mCarouselActivated를 켜고, 선택된 카드를 기준으로 원형 캐러셀 배치를 계산한다.
	 */
	if (ItemIndex < 0 || ItemIndex >= GetClampedCarouselItemDisplayCount())
	{
		return;
	}

	const int32 PreviousIndex = mSelectedCarouselIndex;
	const bool bWasCarouselActivated = mCarouselActivated;
	mCarouselActivated = true;
	mSelectedCarouselIndex = ItemIndex;
	if (PreviousIndex != mSelectedCarouselIndex || !bWasCarouselActivated)
	{
		HandleCarouselSelectionChanged(PreviousIndex, mSelectedCarouselIndex);
	}
	ApplyCarouselLayout(bWasCarouselActivated);
}

void UCarouselPanelWidget::ApplyCarouselLayout(bool bAnimate)
{
	/*
	 * 실제 카드 디자인은 WBP가 담당하고, C++은 계산된 카드 묶음의 배치와 깊이감만 적용한다.
	 */
	const int32 ItemCount = GetClampedCarouselItemDisplayCount();
	if (ItemCount > 0 && (mSelectedCarouselIndex == INDEX_NONE || mSelectedCarouselIndex >= ItemCount))
	{
		mSelectedCarouselIndex = 0;
	}

	TArray<FCarouselItemLayout> TargetLayouts;
	BuildCarouselLayouts(TargetLayouts);
	if (bAnimate == true && mCarouselActivated == true && mCarouselTransitionDuration > 0.0f)
	{
		StartCarouselLayoutTransition(TargetLayouts);
		return;
	}

	mCarouselTransitionActive = false;
	ApplyCarouselLayouts(TargetLayouts);
}

void UCarouselPanelWidget::ResetCarouselState()
{
	/*
	 * 열릴 때마다 선택/눌림 상태를 초기화한다.
	 * 이전 팝업 세션의 Press 좌표가 남으면 첫 터치가 잘못 선택 처리될 수 있다.
	 */
	mPressedCarouselIndex = INDEX_NONE;
	mCarouselPressPosition = FVector2D::ZeroVector;
	mCarouselActivated = false;
	mSelectedCarouselIndex = GetClampedCarouselItemDisplayCount() <= 0 ? INDEX_NONE : 0;
	mCarouselTransitionActive = false;
	mCarouselTransitionElapsed = 0.0f;
}
