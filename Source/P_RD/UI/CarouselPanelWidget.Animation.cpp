#include "UI/CarouselPanelWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "UI/CarouselLayoutPolicy.h"

void UCarouselPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (mCarouselTransitionActive == false)
	{
		return;
	}

	mCarouselTransitionElapsed += InDeltaTime;
	const float RawAlpha = mCarouselTransitionDuration > 0.0f
		? FMath::Clamp(mCarouselTransitionElapsed / mCarouselTransitionDuration, 0.0f, 1.0f)
		: 1.0f;
	const float Alpha = FCarouselLayoutPolicy::EaseOutCubic(RawAlpha);

	TArray<FCarouselItemLayout> CurrentLayouts;
	CurrentLayouts.SetNum(mCarouselTargetLayouts.Num());
	for (int32 Index = 0; Index < mCarouselTargetLayouts.Num(); ++Index)
	{
		const FCarouselItemLayout StartLayout = mCarouselStartLayouts.IsValidIndex(Index)
			? mCarouselStartLayouts[Index]
			: mCarouselTargetLayouts[Index];
		CurrentLayouts[Index] = FCarouselLayoutPolicy::InterpolateLayout(
			StartLayout,
			mCarouselTargetLayouts[Index],
			Alpha,
			RawAlpha);
	}
	ApplyCarouselLayouts(CurrentLayouts);

	if (RawAlpha >= 1.0f)
	{
		mCarouselTransitionActive = false;
		ApplyCarouselLayouts(mCarouselTargetLayouts);
	}
}

void UCarouselPanelWidget::BuildCarouselLayouts(TArray<FCarouselItemLayout>& OutLayouts) const
{
	const int32 ItemCount = GetClampedCarouselItemDisplayCount();
	TArray<float> ItemAngles;
	if (mCarouselActivated == true)
	{
		ItemAngles.SetNum(ItemCount);
		for (int32 Index = 0; Index < ItemCount; ++Index)
		{
			ItemAngles[Index] = GetCarouselItemAngle(Index);
		}
	}

	FCarouselLayoutPolicy::BuildLayouts(
		mCarouselItems.Num(),
		ItemCount,
		mCarouselActivated,
		mSelectedCarouselIndex,
		ItemAngles,
		OutLayouts);
}

void UCarouselPanelWidget::CaptureCurrentCarouselLayouts(TArray<FCarouselItemLayout>& OutLayouts) const
{
	OutLayouts.Reset();
	OutLayouts.SetNum(mCarouselItems.Num());

	for (int32 Index = 0; Index < mCarouselItems.Num(); ++Index)
	{
		const UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr)
		{
			continue;
		}

		FCarouselItemLayout& Layout = OutLayouts[Index];
		Layout.mVisible = Item->GetVisibility() != ESlateVisibility::Collapsed;
		Layout.mScale = Item->GetRenderTransform().Scale;
		Layout.mAngle = Item->GetRenderTransform().Angle;
		Layout.mOpacity = Item->GetRenderOpacity();
		if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
		{
			Layout.mPosition = CanvasSlot->GetPosition();
			Layout.mSize = CanvasSlot->GetSize();
			Layout.mZOrder = CanvasSlot->GetZOrder();
		}
	}
}

void UCarouselPanelWidget::ApplyCarouselLayouts(const TArray<FCarouselItemLayout>& Layouts)
{
	for (int32 Index = 0; Index < mCarouselItems.Num(); ++Index)
	{
		UWidget* Item = mCarouselItems[Index];
		if (Item == nullptr)
		{
			continue;
		}

		const FCarouselItemLayout Layout = Layouts.IsValidIndex(Index) ? Layouts[Index] : FCarouselItemLayout();
		if (Layout.mVisible == false)
		{
			Item->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		Item->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Item->SetRenderScale(Layout.mScale);
		Item->SetRenderTransformAngle(Layout.mAngle);
		Item->SetRenderOpacity(Layout.mOpacity);
		Item->SetVisibility(ESlateVisibility::Visible);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Item->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(Layout.mPosition);
			CanvasSlot->SetSize(Layout.mSize);
			CanvasSlot->SetZOrder(Layout.mZOrder);
		}
	}
}

void UCarouselPanelWidget::StartCarouselLayoutTransition(const TArray<FCarouselItemLayout>& TargetLayouts)
{
	CaptureCurrentCarouselLayouts(mCarouselStartLayouts);
	mCarouselTargetLayouts = TargetLayouts;
	mCarouselTransitionElapsed = 0.0f;
	mCarouselTransitionActive = true;
}
