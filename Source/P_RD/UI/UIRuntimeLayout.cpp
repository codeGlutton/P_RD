#include "UI/UIRuntimeLayout.h"

#include "Blueprint/WidgetTree.h"
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

FAnchors RDUILayout::GetDesignerGroupRect(UWidgetTree* WidgetTree, FName AnchorWidgetName, const FVector2D& DesignSize, const FAnchors& Fallback)
{
	if (WidgetTree == nullptr || AnchorWidgetName.IsNone() || DesignSize.X <= 0.0f || DesignSize.Y <= 0.0f)
	{
		return Fallback;
	}

	UWidget* AnchorWidget = WidgetTree->FindWidget(AnchorWidgetName);
	UCanvasPanelSlot* CanvasSlot = GetCanvasSlot(AnchorWidget);
	if (CanvasSlot == nullptr)
	{
		return Fallback;
	}

	// 앵커 위젯은 (0,0) 점앵커 + 디자인좌표(좌상단) 오프셋/크기로 둔다.
	// 따라서 슬롯의 위치/크기가 곧 디자인 공간 좌표이며, DesignSize로 나누면 정규화 영역이 된다.
	const FVector2D Position = CanvasSlot->GetPosition();
	const FVector2D Size = CanvasSlot->GetSize();
	if (Size.X <= 0.0f || Size.Y <= 0.0f)
	{
		return Fallback;
	}

	return FAnchors(
		Position.X / DesignSize.X,
		Position.Y / DesignSize.Y,
		(Position.X + Size.X) / DesignSize.X,
		(Position.Y + Size.Y) / DesignSize.Y);
}
