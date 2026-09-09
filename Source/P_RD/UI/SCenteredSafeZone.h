#pragma once

#include "Widgets/Layout/SSafeZone.h"
#include "Layout/LayoutUtils.h"

/** Keep centered HUDs centered when the landscape camera cutout is on one side.
 * SSafeZone still supplies platform insets, DPI conversion and rotation updates.
 */
class SCenteredSafeZone : public SSafeZone
{
public:
	virtual void OnArrangeChildren(const FGeometry& Geometry, FArrangedChildren& Children) const override
	{
		if (!Children.Accepts(GetVisibility())) return;
		const FMargin Margin = CenteredMargin(Geometry.Scale);
		const auto X = AlignChild<Orient_Horizontal>(Geometry.GetLocalSize().X, ChildSlot, Margin);
		const auto Y = AlignChild<Orient_Vertical>(Geometry.GetLocalSize().Y, ChildSlot, Margin);
		Children.AddWidget(Geometry.MakeChild(ChildSlot.GetWidget(),
			FVector2D(X.Offset, Y.Offset), FVector2D(X.Size, Y.Size)));
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScale) const override
	{
		if (ChildSlot.GetWidget()->GetVisibility() == EVisibility::Collapsed) return FVector2D::ZeroVector;
		return SBox::ComputeDesiredSize(LayoutScale) + CenteredMargin(LayoutScale).GetDesiredSize();
	}

private:
	FMargin CenteredMargin(float LayoutScale) const
	{
		const FMargin Safe = GetSafeMargin(LayoutScale);
		const float Horizontal = FMath::Max(Safe.Left, Safe.Right);
		const float Vertical = FMath::Max(Safe.Top, Safe.Bottom);
		return FMargin(Horizontal, Vertical, Horizontal, Vertical);
	}
};
