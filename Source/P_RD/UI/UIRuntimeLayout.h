#pragma once

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

class UWidget;

namespace RDUILayout
{
	P_RD_API UCanvasPanelSlot* GetCanvasSlot(UWidget* Widget);
	P_RD_API void ApplyAnchoredSlot(UWidget* Widget, const FAnchors& Anchors, int32 ZOrder);
	P_RD_API void ApplyFixedSlot(UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);
	P_RD_API void ApplyCenteredSlot(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, int32 ZOrder);
}
