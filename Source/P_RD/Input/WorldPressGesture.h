#pragma once

#include "CoreMinimal.h"

/** @brief 월드 포인터 입력을 탭/롱프레스/드래그 취소로 구분한 결과. */
enum class EWorldPressGestureResult : uint8
{
	None,
	Tap,
	LongPress,
};

/**
 * @brief UI와 독립적인 월드 포인터 제스처 상태기.
 * @details 입력 시작점에서 허용 거리 이상 움직이면 취소하고, 임계시간을 한 번 넘겼을 때만 LongPress를 반환한다.
 */
struct FWorldPressGestureTracker
{
public:
	void Begin(const FVector2D& ScreenPosition)
	{
		mStartScreenPosition = ScreenPosition;
		mElapsed = 0.0f;
		mActive = true;
		mLongPressTriggered = false;
	}

	void Cancel()
	{
		Reset();
	}

	EWorldPressGestureResult Update(
		const FVector2D& ScreenPosition,
		float DeltaTime,
		float LongPressThreshold,
		float MoveTolerance)
	{
		if (mActive == false || mLongPressTriggered)
		{
			return EWorldPressGestureResult::None;
		}

		if (HasMovedPastTolerance(ScreenPosition, MoveTolerance))
		{
			Reset();
			return EWorldPressGestureResult::None;
		}

		mElapsed += FMath::Max(0.0f, DeltaTime);
		if (mElapsed >= FMath::Max(0.0f, LongPressThreshold))
		{
			mLongPressTriggered = true;
			return EWorldPressGestureResult::LongPress;
		}

		return EWorldPressGestureResult::None;
	}

	EWorldPressGestureResult End(const FVector2D& ScreenPosition, float MoveTolerance)
	{
		if (mActive == false)
		{
			return EWorldPressGestureResult::None;
		}

		const bool bWasLongPress = mLongPressTriggered;
		const bool bMoved = HasMovedPastTolerance(ScreenPosition, MoveTolerance);
		Reset();

		return bWasLongPress || bMoved
			? EWorldPressGestureResult::None
			: EWorldPressGestureResult::Tap;
	}

	bool IsActive() const { return mActive; }

private:
	bool HasMovedPastTolerance(const FVector2D& ScreenPosition, float MoveTolerance) const
	{
		const float SafeTolerance = FMath::Max(0.0f, MoveTolerance);
		return FVector2D::DistSquared(mStartScreenPosition, ScreenPosition) > FMath::Square(SafeTolerance);
	}

	void Reset()
	{
		mStartScreenPosition = FVector2D::ZeroVector;
		mElapsed = 0.0f;
		mActive = false;
		mLongPressTriggered = false;
	}

private:
	FVector2D mStartScreenPosition = FVector2D::ZeroVector;
	float mElapsed = 0.0f;
	bool mActive = false;
	bool mLongPressTriggered = false;
};
