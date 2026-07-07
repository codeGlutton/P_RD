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
			// 월드 롱프레스 감지 시작 — threshold 넘게 누르면 유닛 상세(RequestWorldTouch true). 탭은 위에서 이미 보냄.
			mWorldPressing = true;
			mWorldLongPressFired = false;
			mWorldPressElapsed = 0.0f;
			mWorldPressScreenPos = ScreenPosition;
			return FReply::Handled();
		}
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCombatTileMapHUDWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	mWorldPressing = false;   // 손을 뗐으니 월드 롱프레스 추적 종료(더 이상 롱프레스로 승격 안 함).
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || mSkillPressing == false)
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
			// 월드 롱프레스 감지 시작 — threshold 넘게 누르면 유닛 상세(RequestWorldTouch true). 탭은 위에서 이미 보냄.
			mWorldPressing = true;
			mWorldLongPressFired = false;
			mWorldPressElapsed = 0.0f;
			mWorldPressScreenPos = ScreenPosition;
			return FReply::Handled();
		}
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}

	BeginSkillPress(SkillIndex);
	return FReply::Handled();
}

FReply UCombatTileMapHUDWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	mWorldPressing = false;   // 손을 뗐으니 월드 롱프레스 추적 종료.
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

/** @brief 월드(보드) 롱프레스 누적. 탭은 다운에서 이미 보냈고, threshold를 넘기면 롱프레스(true)를 한 번 더 보내 유닛 상세를 연다. */
void UCombatTileMapHUDWidget::UpdateWorldPress(float InDeltaTime)
{
	if (mWorldPressing == false || mWorldLongPressFired == true)
	{
		return;
	}

	mWorldPressElapsed += InDeltaTime;
	if (mWorldPressElapsed >= mSkillLongPressThreshold)
	{
		if (mCombatUIModel != nullptr)
		{
			mCombatUIModel->RequestWorldTouch(mWorldPressScreenPos, true);   // 롱프레스 → 게임플레이 월드 트레이스 → 유닛 히트 시 상세.
		}
		mWorldLongPressFired = true;
	}
}
