#include "UI/TitleMenuButtonPolicy.h"

namespace
{
	constexpr int32 TitleButtonBackgroundZOrder = 11;
	constexpr int32 TitleButtonInputZOrder = 12;

	const FAnchors TitleButtonCanvasAnchors(0.0f, 1.0f, 0.0f, 1.0f);
	const FVector2D TitleButtonCanvasAlignment(0.0f, 1.0f);
	const FVector2D MainButtonSize(360.0f, 136.0f);
	const FVector2D SettingsButtonSize(320.0f, 126.0f);
}

void RDTitleMenuButton::BuildButtonLayouts(TArray<FButtonLayout>& OutLayouts)
{
	OutLayouts.Reset();
	OutLayouts.Reserve(3);
	OutLayouts.Add({ EButtonSlot::Continue, FVector2D(70.0f, -320.0f), MainButtonSize });
	OutLayouts.Add({ EButtonSlot::Start, FVector2D(70.0f, -185.0f), MainButtonSize });
	OutLayouts.Add({ EButtonSlot::Settings, FVector2D(90.0f, -60.0f), SettingsButtonSize });
}

int32 RDTitleMenuButton::GetBackgroundZOrder()
{
	return TitleButtonBackgroundZOrder;
}

int32 RDTitleMenuButton::GetInputZOrder()
{
	return TitleButtonInputZOrder;
}

FAnchors RDTitleMenuButton::GetCanvasAnchors()
{
	return TitleButtonCanvasAnchors;
}

FVector2D RDTitleMenuButton::GetCanvasAlignment()
{
	return TitleButtonCanvasAlignment;
}

FLinearColor RDTitleMenuButton::GetHoveredTintColor()
{
	return FLinearColor(1.08f, 1.06f, 1.0f, 1.0f);
}

FLinearColor RDTitleMenuButton::GetPressedTintColor()
{
	return FLinearColor(0.74f, 0.82f, 0.82f, 1.0f);
}

FLinearColor RDTitleMenuButton::GetDisabledTintColor()
{
	return FLinearColor(0.45f, 0.48f, 0.50f, 0.62f);
}

FMargin RDTitleMenuButton::GetNormalPadding()
{
	return FMargin(0.0f);
}

FMargin RDTitleMenuButton::GetPressedPadding()
{
	return FMargin(1.0f, 2.0f, 0.0f, 0.0f);
}

FVector2D RDTitleMenuButton::GetFallbackButtonPivot()
{
	return FVector2D(0.5f, 0.5f);
}

FVector2D RDTitleMenuButton::GetFallbackButtonScale()
{
	return FVector2D(1.0f, 1.9f);
}

FSlateColor RDTitleMenuButton::GetButtonTextColor()
{
	return FSlateColor(FLinearColor(0.92f, 0.83f, 0.58f, 1.0f));
}

FLinearColor RDTitleMenuButton::GetButtonShadowColor()
{
	return FLinearColor(0.0f, 0.0f, 0.0f, 0.82f);
}

FVector2D RDTitleMenuButton::GetButtonShadowOffset()
{
	return FVector2D(1.0f, 2.0f);
}
