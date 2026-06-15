#include "UI/UIRuntimeLayout.h"

#include "Components/Widget.h"

// CanvasPanel에 붙은 위젯만 위치와 크기를 바꿀 수 있다.
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

	// 이전 배치값이 남지 않게 기본값으로 돌린 뒤, Anchor 영역을 꽉 채운다.
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

	// 고정 크기 위젯에 필요한 위치/크기 값을 한 번에 적용한다.
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetPosition(Position);
	CanvasSlot->SetSize(Size);
	CanvasSlot->SetZOrder(ZOrder);
}

void RDUILayout::ApplyCenteredSlot(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, int32 ZOrder)
{
	// 중앙 배치는 항상 같은 Anchor/Alignment를 쓰므로 여기서 대신 넣어 준다.
	ApplyFixedSlot(Widget, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), Position, Size, ZOrder);
}
