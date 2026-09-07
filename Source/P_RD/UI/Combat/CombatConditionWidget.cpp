#include "UI/Combat/CombatConditionWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"

namespace
{
class SConditionFace final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SConditionFace) {} SLATE_ATTRIBUTE(EUnitCombatCondition, Condition) SLATE_END_ARGS()
	void Construct(const FArguments& Args) { Condition = Args._Condition; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(44.f,44.f); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool) const override
	{
		const auto Value = Condition.Get(EUnitCombatCondition::Normal);
		const float Size = FMath::Min(Geometry.GetLocalSize().X, Geometry.GetLocalSize().Y);
		const FVector2D Origin = (Geometry.GetLocalSize()-FVector2D(Size))*.5f;
		const FLinearColor Tint = Style.GetColorAndOpacityTint();
		FLinearColor Color(.56f,.64f,.73f);
		if (Value == EUnitCombatCondition::Bad) Color = FLinearColor(.88f,.32f,.21f);
		if (Value == EUnitCombatCondition::Good) Color = FLinearColor(.22f,.79f,.51f);
		if (Value == EUnitCombatCondition::Excellent) Color = FLinearColor(1.f,.73f,.19f);
		const FLinearColor Ink(.035f,.045f,.055f);
		auto Disc = [&](float Inset, FLinearColor C, int32 Z)
		{
			FSlateDrawElement::MakeBox(Out,Z,Geometry.ToPaintGeometry(FVector2D(Size-2*Inset),FSlateLayoutTransform(Origin+FVector2D(Inset))),&Round,ESlateDrawEffect::None,C*Tint);
		};
		Disc(0.f,Ink,Layer); Disc(Size*.045f,FLinearColor(.73f,.60f,.36f),Layer+1);
		Disc(Size*.095f,Color,Layer+2);
		auto Line = [&](const TArray<FVector2D>& Points, float Width=0.065f)
		{
			TArray<FVector2D> Scaled;for (const auto& P:Points) Scaled.Add(Origin+P*Size);
			FSlateDrawElement::MakeLines(Out,Layer+3,Geometry.ToPaintGeometry(),Scaled,ESlateDrawEffect::None,Ink*Tint,true,FMath::Max(1.5f,Width*Size));
		};
		for (float X : {.34f,.66f})
		{
			if (Value == EUnitCombatCondition::Excellent)
				Line({{X-.075f,.43f},{X,.34f},{X+.075f,.43f}},.06f);
			else Line({{X,.36f},{X,.47f}},.075f);
		}
		TArray<FVector2D> Mouth;
		for (int32 I=0;I<=16;++I)
		{
			const float T=I/16.f;
			const float Bend = Value==EUnitCombatCondition::Bad ? -.115f : Value==EUnitCombatCondition::Normal ? 0.f : .13f;
			Mouth.Add(FVector2D(.29f+.42f*T,.66f+Bend*FMath::Sin(PI*T)));
		}
		Line(Mouth);
		return Layer+3;
	}
private:
	TAttribute<EUnitCombatCondition> Condition;
	// Let Slate derive the radius from each disc's height, including DPI scaling.
	FSlateRoundedBoxBrush Round{FLinearColor::White};
};
}

FText UCombatConditionWidget::Describe(EUnitCombatCondition Value)
{
	switch(Value)
	{
	case EUnitCombatCondition::Bad:return NSLOCTEXT("CombatCondition","Bad","컨디션: 나쁨 · 최소 피해");
	case EUnitCombatCondition::Good:return NSLOCTEXT("CombatCondition","Good","컨디션: 좋음 · 최대 피해");
	case EUnitCombatCondition::Excellent:return NSLOCTEXT("CombatCondition","Excellent","컨디션: 최상 · 최대 피해 + 치명타");
	default:return NSLOCTEXT("CombatCondition","Normal","컨디션: 보통 · 평균 피해");
	}
}
void UCombatConditionWidget::SetCondition(EUnitCombatCondition Value)
{
	Condition = Value < EUnitCombatCondition::Count ? Value : EUnitCombatCondition::Normal;
	SetToolTipText(Describe(Condition));
	if(Face.IsValid()) Face->Invalidate(EInvalidateWidgetReason::Paint);
}
TSharedRef<SWidget> UCombatConditionWidget::RebuildWidget()
{
	Face=SNew(SConditionFace).Condition_Lambda([this](){return Condition;});
	return Face.ToSharedRef();
}
void UCombatConditionWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);Face.Reset();
}
