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

	// 앵커 위젯은 점앵커 + 디자인px 오프셋/크기로 둔다. 레거시는 (0,0) 점앵커 절대좌표,
	// 엣지 피닝 스킨은 (ax,ay) 점앵커 상대좌표 — 디자인 기준 절대좌표로 환산해 정규화한다(진단/레거시 경로용).
	const FAnchors SlotAnchors = CanvasSlot->GetAnchors();
	FVector2D Position = CanvasSlot->GetPosition();
	const FVector2D Size = CanvasSlot->GetSize();
	if (Size.X <= 0.0f || Size.Y <= 0.0f)
	{
		return Fallback;
	}
	if (SlotAnchors.Minimum == SlotAnchors.Maximum)
	{
		Position.X += SlotAnchors.Minimum.X * DesignSize.X;
		Position.Y += SlotAnchors.Minimum.Y * DesignSize.Y;
	}

	return FAnchors(
		Position.X / DesignSize.X,
		Position.Y / DesignSize.Y,
		(Position.X + Size.X) / DesignSize.X,
		(Position.Y + Size.Y) / DesignSize.Y);
}

bool RDUILayout::GetDesignerSlotData(UWidgetTree* WidgetTree, FName MarkerName, FAnchorData& OutSlotData)
{
	if (WidgetTree == nullptr || MarkerName.IsNone())
	{
		return false;
	}
	UCanvasPanelSlot* CanvasSlot = GetCanvasSlot(WidgetTree->FindWidget(MarkerName));
	if (CanvasSlot == nullptr)
	{
		return false;
	}
	OutSlotData = CanvasSlot->GetLayout();
	return true;
}

FAnchorData RDUILayout::NormalizedToDesignPointSlot(const FAnchors& Normalized, const FVector2D& DesignSize)
{
	FAnchorData Out;
	Out.Anchors = FAnchors(0.0f, 0.0f, 0.0f, 0.0f);
	Out.Alignment = FVector2D::ZeroVector;
	Out.Offsets = FMargin(
		Normalized.Minimum.X * DesignSize.X,
		Normalized.Minimum.Y * DesignSize.Y,
		(Normalized.Maximum.X - Normalized.Minimum.X) * DesignSize.X,
		(Normalized.Maximum.Y - Normalized.Minimum.Y) * DesignSize.Y);
	return Out;
}

FAnchorData RDUILayout::GetDesignerSlotDataOr(UWidgetTree* WidgetTree, FName MarkerName, const FAnchors& FallbackNormalized, const FVector2D& DesignSize)
{
	FAnchorData SlotData;
	if (GetDesignerSlotData(WidgetTree, MarkerName, SlotData))
	{
		return SlotData;
	}
	return NormalizedToDesignPointSlot(FallbackNormalized, DesignSize);
}

void RDUILayout::ApplyDesignerSlotData(UWidget* Widget, const FAnchorData& SlotData, int32 ZOrder)
{
	UCanvasPanelSlot* CanvasSlot = GetCanvasSlot(Widget);
	if (CanvasSlot == nullptr)
	{
		return;
	}
	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetLayout(SlotData);
	CanvasSlot->SetZOrder(ZOrder);
}

TArray<FAnchorData> RDUILayout::CollectPointSlotsByPrefix(UWidgetTree* WidgetTree, const FString& NamePrefix)
{
	TArray<FAnchorData> Out;
	if (WidgetTree == nullptr)
	{
		return Out;
	}
	WidgetTree->ForEachWidget([&Out, &NamePrefix](UWidget* Widget) {
		if (Widget == nullptr || Widget->GetName().StartsWith(NamePrefix) == false)
		{
			return;
		}
		if (UCanvasPanelSlot* CanvasSlot = GetCanvasSlot(Widget))
		{
			Out.Add(CanvasSlot->GetLayout());
		}
		});
	Out.Sort([](const FAnchorData& Lhs, const FAnchorData& Rhs) {
		return Lhs.Offsets.Top < Rhs.Offsets.Top;
		});
	return Out;
}

FAnchorData RDUILayout::MakeVerticalSubSlot(const FAnchorData& GroupSlot, int32 Index, int32 Count, float GapPx)
{
	FAnchorData Out = GroupSlot;
	if (Count <= 0)
	{
		return Out;
	}
	// 점앵커 슬롯 전제: Offsets.Right/Bottom = 크기(px).
	const float ItemHeight = (GroupSlot.Offsets.Bottom - StaticCast<float>(Count - 1) * GapPx) / StaticCast<float>(Count);
	Out.Offsets.Top += StaticCast<float>(Index) * (ItemHeight + GapPx);
	Out.Offsets.Bottom = ItemHeight;
	return Out;
}
