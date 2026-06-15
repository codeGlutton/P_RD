#pragma once

#include "RDMinimal.h"

struct P_RD_API FCarouselItemLayout
{
	bool mVisible = false;
	FVector2D mPosition = FVector2D::ZeroVector;
	FVector2D mSize = FVector2D::ZeroVector;
	FVector2D mScale = FVector2D(1.0f, 1.0f);
	float mAngle = 0.0f;
	float mOpacity = 1.0f;
	int32 mZOrder = 0;
};

struct P_RD_API FCarouselLayoutPolicy
{
	static constexpr int32 MaxItemCount = 8;
	static constexpr float TapDistanceThreshold = 28.0f;

	static void BuildLayouts(
		int32 TotalItemCount,
		int32 VisibleItemCount,
		bool bActivated,
		int32 SelectedItemIndex,
		const TArray<float>& ItemAngles,
		TArray<FCarouselItemLayout>& OutLayouts);

	static float EaseOutCubic(float RawAlpha);
	static FCarouselItemLayout InterpolateLayout(
		const FCarouselItemLayout& StartLayout,
		const FCarouselItemLayout& TargetLayout,
		float Alpha,
		float RawAlpha);
};
