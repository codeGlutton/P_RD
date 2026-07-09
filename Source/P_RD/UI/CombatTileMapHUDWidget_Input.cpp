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

	// LV 칸 누름: 누르는 동안 LV 아래 회색 패널로 경험치를 보여준다(손가락이 LV를 가려도 보이게). 월드 터치로 넘기지 않는다.
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
			mCombatUIModel->RequestWorldTouch(ScreenPosition, false);
			return FReply::Handled();
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

	// LV 칸에서 손을 떼면 경험치 패널을 닫는다.
	if (mLevelValueTouched)
	{
		mLevelValueTouched = false;
		SetExpHoldPanelVisible(false);
		return FReply::Handled();
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

	// LV 칸 누름: 누르는 동안 LV 아래 회색 패널로 경험치를 보여준다(손가락이 LV를 가려도 보이게). 월드 터치로 넘기지 않는다.
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
			mCombatUIModel->RequestWorldTouch(ScreenPosition, false);
			return FReply::Handled();
		}
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	// LV 칸에서 손을 떼면 경험치 패널을 닫는다.
	if (mLevelValueTouched)
	{
		mLevelValueTouched = false;
		SetExpHoldPanelVisible(false);
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
