#include "UI/UIRuntimeLayout.h"

#include "Components/Widget.h"

// 위젯의 Slot은 부모가 CanvasPanel일 때만 UCanvasPanelSlot이다.
// 다른 패널(VerticalBox 등)에 붙어 있으면 Cast가 실패하므로, 그 경우는 배치를 건드리지 않고
// nullptr을 돌려준다. 호출부가 슬롯 종류를 매번 확인하지 않아도 되게 하기 위한 단일 관문이다.
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

	// Anchor 영역을 그대로 채우는 용도이므로 AutoSize를 끄고 Offset/Alignment를 0으로 되돌린다.
	// 같은 위젯을 ApplyFixedSlot 등으로 재배치했다가 다시 쓰는 경우, 이전 Alignment가 남아
	// 채움이 어긋나는 것을 막기 위해 매번 명시적으로 기본값으로 초기화한다.
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

	// 크기가 고정된 위젯이므로 AutoSize를 끄고 호출부가 넘긴 Anchor/Alignment/Position/Size를
	// 그대로 적용한다. 배치에 필요한 값을 한 번에 설정해, 호출부마다 슬롯 속성을 따로 만지는 코드가
	// 흩어지지 않게 한다.
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetPosition(Position);
	CanvasSlot->SetSize(Size);
	CanvasSlot->SetZOrder(ZOrder);
}

void RDUILayout::ApplyCenteredSlot(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, int32 ZOrder)
{
	// "화면 중앙 정렬"은 Anchor(0.5, 0.5) + Alignment(0.5, 0.5) 조합으로 늘 같다.
	// 팝업/중앙 오버레이를 띄우는 호출부가 이 매직 값을 반복해서 쓰지 않도록 ApplyFixedSlot에 위임한다.
	ApplyFixedSlot(Widget, FAnchors(0.5f, 0.5f), FVector2D(0.5f, 0.5f), Position, Size, ZOrder);
}
