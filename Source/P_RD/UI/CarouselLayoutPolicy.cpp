#include "UI/CarouselLayoutPolicy.h"

namespace
{
	const FVector2D CarouselOrigin(0.0f, 36.0f);
	const FVector2D CarouselBaseSize(180.0f, 244.0f);
	const FVector2D LinearListOrigin(0.0f, 36.0f);
	constexpr float CarouselRadiusX = 360.0f;
	constexpr float CarouselRadiusY = 58.0f;
	constexpr float LinearListMaxWidth = 920.0f;
	constexpr float LinearListPreferredSpacing = 168.0f;
	constexpr float LinearListScale = 0.74f;
	constexpr float MinCarouselScale = 0.44f;
	constexpr float MaxCarouselScale = 1.18f;
	constexpr float MinCarouselOpacity = 0.30f;
	constexpr float MaxCarouselOpacity = 1.00f;

	void BuildLinearListLayouts(int32 VisibleItemCount, TArray<FCarouselItemLayout>& OutLayouts)
	{
		const float Spacing = VisibleItemCount > 1
			? FMath::Min(LinearListPreferredSpacing, LinearListMaxWidth / StaticCast<float>(VisibleItemCount - 1))
			: 0.0f;
		const float StartX = -Spacing * StaticCast<float>(VisibleItemCount - 1) * 0.5f;

		for (int32 Index = 0; Index < VisibleItemCount; ++Index)
		{
			FCarouselItemLayout& Layout = OutLayouts[Index];
			Layout.mVisible = true;
			Layout.mPosition = LinearListOrigin + FVector2D(StartX + Spacing * Index, 0.0f);
			Layout.mSize = CarouselBaseSize;
			Layout.mScale = FVector2D(LinearListScale, LinearListScale);
			Layout.mAngle = 0.0f;
			Layout.mOpacity = 1.0f;
			Layout.mZOrder = 100 + Index;
		}
	}

	void BuildActivatedCarouselLayouts(
		int32 VisibleItemCount,
		int32 SelectedItemIndex,
		const TArray<float>& ItemAngles,
		TArray<FCarouselItemLayout>& OutLayouts)
	{
		const int32 SelectedIndex = (SelectedItemIndex < 0 || SelectedItemIndex >= VisibleItemCount) ? 0 : SelectedItemIndex;
		for (int32 Index = 0; Index < VisibleItemCount; ++Index)
		{
			const int32 RelativeIndex = (Index - SelectedIndex + VisibleItemCount) % VisibleItemCount;
			const float Angle = 2.0f * PI * StaticCast<float>(RelativeIndex) / StaticCast<float>(VisibleItemCount);
			const float Depth = FMath::Cos(Angle);
			const float NormalizedDepth = (Depth + 1.0f) * 0.5f;
			const float X = FMath::Sin(Angle) * CarouselRadiusX;
			const float Y = Depth * CarouselRadiusY;
			const float Scale = FMath::Lerp(MinCarouselScale, MaxCarouselScale, NormalizedDepth);
			const float Opacity = FMath::Lerp(MinCarouselOpacity, MaxCarouselOpacity, NormalizedDepth);
			const int32 ZOrder = FMath::RoundToInt(100.0f + Depth * 100.0f);

			FCarouselItemLayout& Layout = OutLayouts[Index];
			Layout.mVisible = true;
			Layout.mPosition = CarouselOrigin + FVector2D(X, Y);
			Layout.mSize = CarouselBaseSize;
			Layout.mScale = FVector2D(Scale, Scale);
			Layout.mAngle = ItemAngles.IsValidIndex(Index) ? ItemAngles[Index] : 0.0f;
			Layout.mOpacity = Opacity;
			Layout.mZOrder = ZOrder;
		}
	}
}

void FCarouselLayoutPolicy::BuildLayouts(
	int32 TotalItemCount,
	int32 VisibleItemCount,
	bool bActivated,
	int32 SelectedItemIndex,
	const TArray<float>& ItemAngles,
	TArray<FCarouselItemLayout>& OutLayouts)
{
	OutLayouts.Reset();
	OutLayouts.SetNum(FMath::Max(0, TotalItemCount));

	const int32 ClampedVisibleItemCount = FMath::Clamp(VisibleItemCount, 0, OutLayouts.Num());
	if (ClampedVisibleItemCount <= 0)
	{
		return;
	}

	if (bActivated == false)
	{
		BuildLinearListLayouts(ClampedVisibleItemCount, OutLayouts);
		return;
	}

	BuildActivatedCarouselLayouts(ClampedVisibleItemCount, SelectedItemIndex, ItemAngles, OutLayouts);
}

float FCarouselLayoutPolicy::EaseOutCubic(float RawAlpha)
{
	const float ClampedAlpha = FMath::Clamp(RawAlpha, 0.0f, 1.0f);
	const float InverseAlpha = 1.0f - ClampedAlpha;
	return 1.0f - InverseAlpha * InverseAlpha * InverseAlpha;
}

FCarouselItemLayout FCarouselLayoutPolicy::InterpolateLayout(
	const FCarouselItemLayout& StartLayout,
	const FCarouselItemLayout& TargetLayout,
	float Alpha,
	float RawAlpha)
{
	FCarouselItemLayout ResolvedStartLayout = StartLayout;
	FCarouselItemLayout ResolvedTargetLayout = TargetLayout;

	if (ResolvedStartLayout.mVisible == false && ResolvedTargetLayout.mVisible == true)
	{
		ResolvedStartLayout = ResolvedTargetLayout;
		ResolvedStartLayout.mOpacity = 0.0f;
	}
	else if (ResolvedStartLayout.mVisible == true && ResolvedTargetLayout.mVisible == false)
	{
		ResolvedTargetLayout = ResolvedStartLayout;
		ResolvedTargetLayout.mOpacity = 0.0f;
	}

	FCarouselItemLayout CurrentLayout;
	CurrentLayout.mVisible = ResolvedStartLayout.mVisible || ResolvedTargetLayout.mVisible;
	CurrentLayout.mPosition = ResolvedStartLayout.mPosition + (ResolvedTargetLayout.mPosition - ResolvedStartLayout.mPosition) * Alpha;
	CurrentLayout.mSize = ResolvedStartLayout.mSize + (ResolvedTargetLayout.mSize - ResolvedStartLayout.mSize) * Alpha;
	CurrentLayout.mScale = ResolvedStartLayout.mScale + (ResolvedTargetLayout.mScale - ResolvedStartLayout.mScale) * Alpha;
	CurrentLayout.mAngle = FMath::Lerp(ResolvedStartLayout.mAngle, ResolvedTargetLayout.mAngle, Alpha);
	CurrentLayout.mOpacity = FMath::Lerp(ResolvedStartLayout.mOpacity, ResolvedTargetLayout.mOpacity, Alpha);
	CurrentLayout.mZOrder = RawAlpha < 0.5f ? ResolvedStartLayout.mZOrder : ResolvedTargetLayout.mZOrder;
	return CurrentLayout;
}
