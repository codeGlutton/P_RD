#include "UI/CombatTileMapHUDWidget.h"

#include "InputCoreTypes.h"
#include "UI/Combat/CombatUIModel.h"

FReply UCombatTileMapHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	if (IsScreenPositionOverLevelValue(ScreenPosition))
	{
		mLevelValueTouched = true;
		SetExpHoldPanelVisible(true);
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	const bool bClosedSkillDetail = HideSkillDetailIfClickedOutside(ScreenPosition);
	const int32 SkillIndex = FindSkillRailIndexAtScreenPosition(ScreenPosition);
	if (SkillIndex == INDEX_NONE)
	{
		if (bClosedSkillDetail == true)
		{
			return FReply::Handled();
		}
		// 스킬 레일 밖 = 월드(타일/유닛) 영역. 뷰모델 연결 시 좌표만 넘기고 타일/유닛 판정은 게임플레이가.
		if (mCombatUIModel != nullptr)
		{
			BeginWorldPress(ScreenPosition);
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCombatTileMapHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (mLevelValueTouched)
	{
		mLevelValueTouched = false;
		SetExpHoldPanelVisible(false);
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (mWorldPressing)
	{
		EndWorldPress();
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (mSkillPressing == false)
	{
		return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	}

	if (mSkillDetailOpenedFromPress == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillDetailOpenedFromPress = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	const FVector2D ScreenPosition = InGestureEvent.GetScreenSpacePosition();
	if (IsScreenPositionOverLevelValue(ScreenPosition))
	{
		mLevelValueTouched = true;
		SetExpHoldPanelVisible(true);
		return FReply::Handled();
	}

	const bool bClosedSkillDetail = HideSkillDetailIfClickedOutside(ScreenPosition);
	const int32 SkillIndex = FindSkillRailIndexAtScreenPosition(ScreenPosition);
	if (SkillIndex == INDEX_NONE)
	{
		if (bClosedSkillDetail == true)
		{
			return FReply::Handled();
		}
		// 스킬 레일 밖 = 월드(타일/유닛) 영역. 뷰모델 연결 시 좌표만 넘기고 타일/유닛 판정은 게임플레이가.
		if (mCombatUIModel != nullptr)
		{
			BeginWorldPress(ScreenPosition);
			return FReply::Handled();
		}
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mLevelValueTouched)
	{
		mLevelValueTouched = false;
		SetExpHoldPanelVisible(false);
		return FReply::Handled();
	}

	if (mWorldPressing)
	{
		EndWorldPress();
		return FReply::Handled();
	}

	if (mSkillPressing == false)
	{
		return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
	}

	if (mSkillDetailOpenedFromPress == false)
	{
		SelectSkillForAssignment(mPressedSkillIndex);
	}

	mSkillPressing = false;
	mPressedSkillIndex = INDEX_NONE;
	mSkillPressElapsed = 0.0f;
	mSkillDetailOpenedFromPress = false;
	return FReply::Handled();
}

void UCombatTileMapHUDWidget::BeginWorldPress(const FVector2D& ScreenPosition)
{
	mWorldPressing = true;
	mWorldLongPressTriggered = false;
	mWorldPressElapsed = 0.0f;
	mWorldPressScreenPosition = ScreenPosition;
}

void UCombatTileMapHUDWidget::UpdateWorldPress(float InDeltaTime)
{
	if (mWorldPressing == false || mWorldLongPressTriggered == true || mCombatUIModel == nullptr)
	{
		return;
	}

	mWorldPressElapsed += InDeltaTime;
	if (mWorldPressElapsed < mSkillLongPressThreshold)
	{
		return;
	}

	// OnCombatWorldTouch와 그 안의 SetUnitDetail/OnUIChanged는 동기 호출이다. 이 구간의 Unit 갱신만
	// 상세 팝업으로 열어 일반 HP/상태 갱신이 오래된 상세를 다시 띄우지 않게 한다.
	mWorldLongPressTriggered = true;
	mWorldLongPressAwaitingDetail = true;
	mCombatUIModel->RequestWorldTouch(mWorldPressScreenPosition, true);
	mWorldLongPressAwaitingDetail = false;
}

void UCombatTileMapHUDWidget::EndWorldPress()
{
	const bool bShouldSendTap = mWorldPressing && mWorldLongPressTriggered == false;
	const FVector2D ScreenPosition = mWorldPressScreenPosition;

	mWorldPressing = false;
	mWorldLongPressTriggered = false;
	mWorldPressElapsed = 0.0f;
	mWorldPressScreenPosition = FVector2D::ZeroVector;
	mWorldLongPressAwaitingDetail = false;

	if (bShouldSendTap && mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestWorldTouch(ScreenPosition, false);
	}
}
