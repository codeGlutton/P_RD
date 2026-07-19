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
		// 방향을 고르지 않는 공격/방해는 한 번 탭으로 끝낸다. 손아귀만 포인터를 캡처해 드래그한다.
		if (TryExecuteTapSkillAtScreenPosition(ScreenPosition))
		{
			return FReply::Handled();
		}
		if (BeginDirectUnitGesture(ScreenPosition))
		{
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
		// 유닛 바깥 = 기존 월드(타일) 영역. 좌표 판정은 게임플레이가 담당한다.
		if (mCombatUIModel != nullptr)
		{
			mCombatUIModel->RequestWorldTouch(ScreenPosition, false);
			return FReply::Handled();
		}
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCombatTileMapHUDWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (mDirectUnitGestureActive)
	{
		UpdateDirectUnitGesture(InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
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

	if (mDirectUnitGestureActive)
	{
		EndDirectUnitGesture(InMouseEvent.GetScreenSpacePosition());
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
		// 마우스 경로와 동일하게 포인터를 캡처한다 — 캡처가 없으면 누른 채 다른 버튼 위로 끌고 가 뗐을 때
		// 해제 이벤트가 그 버튼으로 가버려 mLevelValueTouched가 끼고 경험치 패널이 화면에 남는다.
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
		// 방향을 고르지 않는 공격/방해는 한 번 탭으로 끝낸다. 손아귀만 포인터를 캡처해 드래그한다.
		if (TryExecuteTapSkillAtScreenPosition(ScreenPosition))
		{
			return FReply::Handled();
		}
		if (BeginDirectUnitGesture(ScreenPosition))
		{
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
		if (mCombatUIModel != nullptr)
		{
			mCombatUIModel->RequestWorldTouch(ScreenPosition, false);
			return FReply::Handled();
		}
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}

	BeginSkillPress(SkillIndex);
	// 마우스 경로(NativeOnMouseButtonDown)와 동일하게 캡처 — 레일 밖에서 뗀 해제도 이 위젯이 받는다.
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCombatTileMapHUDWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mDirectUnitGestureActive)
	{
		UpdateDirectUnitGesture(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}
	return Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
}

FReply UCombatTileMapHUDWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (mLevelValueTouched)
	{
		mLevelValueTouched = false;
		SetExpHoldPanelVisible(false);
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (mDirectUnitGestureActive)
	{
		EndDirectUnitGesture(InGestureEvent.GetScreenSpacePosition());
		return FReply::Handled().ReleaseMouseCapture();
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
	return FReply::Handled().ReleaseMouseCapture();
}
