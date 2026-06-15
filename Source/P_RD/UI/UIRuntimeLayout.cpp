#include "UI/UIRuntimeLayout.h"

/**
 * @file UIRuntimeLayout.cpp
 * @brief 코드로 만든 UI의 Canvas 배치를 적용합니다.
 *
 * 위젯이 없거나 CanvasPanel에 붙어 있지 않은 경우는 여기서 걸러냅니다.
 * HUD 코드는 배치하고 싶은 값만 넘기면 됩니다.
 */

#include "Components/Widget.h"

UCanvasPanelSlot* RDUILayout::GetCanvasSlot(UWidget* Widget)
{
	return Widget != nullptr ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
}

void RDUILayout::ApplyAnchoredSlot(UWidget* Widget, const FAnchors& Anchors, int32 ZOrder)
{
	UCanvasPanelSlot* CanvasSlot = GetCanvasSlot(Widget);
	if (CanvasSlot == nullptr)
	{
		return;
	}

	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetOffsets(FMargin(0.0f));
	CanvasSlot->SetAlignment(FVector2D::ZeroVector);
	CanvasSlot->SetZOrder(ZOrder);
}

void RDUILayout::ApplyFixedSlot(UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position, const FVector2D& Size, int32 ZOrder)
{
	UCanvasPanelSlot* CanvasSlot = GetCanvasSlot(Widget);
	if (CanvasSlot == nullptr)
	{
		return;
	}

	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetPosition(Position);
	CanvasSlot->SetSize(Size);
	CanvasSlot->SetZOrder(ZOrder);
}

void RDUILayout::ApplyCenteredSlot(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, int32 ZOrder)
{
	ApplyFixedSlot(Widget, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), Position, Size, ZOrder);
}
