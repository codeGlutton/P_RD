#pragma once

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

namespace RDTitleMenuButton
{
	enum class EButtonSlot : uint8
	{
		Continue,
		Start,
		Settings
	};

	struct P_RD_API FButtonLayout
	{
		EButtonSlot mSlot = EButtonSlot::Start;
		FVector2D mPosition = FVector2D::ZeroVector;
		FVector2D mSize = FVector2D::ZeroVector;
	};

	P_RD_API void BuildButtonLayouts(TArray<FButtonLayout>& OutLayouts);
	P_RD_API int32 GetBackgroundZOrder();
	P_RD_API int32 GetInputZOrder();
	P_RD_API FAnchors GetCanvasAnchors();
	P_RD_API FVector2D GetCanvasAlignment();
	P_RD_API FLinearColor GetHoveredTintColor();
	P_RD_API FLinearColor GetPressedTintColor();
	P_RD_API FLinearColor GetDisabledTintColor();
	P_RD_API FMargin GetNormalPadding();
	P_RD_API FMargin GetPressedPadding();
	P_RD_API FVector2D GetFallbackButtonPivot();
	P_RD_API FVector2D GetFallbackButtonScale();
	P_RD_API FSlateColor GetButtonTextColor();
	P_RD_API FLinearColor GetButtonShadowColor();
	P_RD_API FVector2D GetButtonShadowOffset();
}
