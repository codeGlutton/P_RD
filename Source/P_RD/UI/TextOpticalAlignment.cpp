#include "UI/TextOpticalAlignment.h"

#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Fonts/FontCache.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/TextTransformer.h"
#include "Rendering/SlateRenderer.h"

namespace RDTextOpticalAlignment
{
	TOptional<float> MeasureOffsetY(const UTextBlock& TextBlock)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return {};
		}

		FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
		if (Renderer == nullptr)
		{
			return {};
		}

		FString DisplayString = TextBlock.GetText().ToString();
		switch (TextBlock.GetTextTransformPolicy())
		{
		case ETextTransformPolicy::ToLower:
			DisplayString = FTextTransformer::ToLower(DisplayString);
			break;
		case ETextTransformPolicy::ToUpper:
			DisplayString = FTextTransformer::ToUpper(DisplayString);
			break;
		default:
			break;
		}

		if (DisplayString.IsEmpty())
		{
			return 0.0f;
		}

		const FSlateFontInfo& FontInfo = TextBlock.GetFont();
		const TSharedRef<FSlateFontCache> FontCache = Renderer->GetFontCache();
		const TextBiDi::ETextDirection BaseDirection =
			TextBiDi::ComputeBaseDirection(DisplayString);
		const FShapedGlyphSequenceRef GlyphSequence =
			FontCache->ShapeBidirectionalText(DisplayString, FontInfo, 1.0f,
				BaseDirection, GetDefaultTextShapingMethod());

		const float MaxTextHeight = static_cast<float>(GlyphSequence->GetMaxTextHeight());
		const float TextBaseline = static_cast<float>(GlyphSequence->GetTextBaseline());
		float InkTop = TNumericLimits<float>::Max();
		float InkBottom = -TNumericLimits<float>::Max();
		bool bFoundVisibleGlyph = false;

		for (const FShapedGlyphEntry& Glyph : GlyphSequence->GetGlyphsToRender())
		{
			if (!Glyph.bIsVisible)
			{
				continue;
			}

			// 외곽선은 중심을 바꾸지 않고 위아래로 같은 양만 늘어나므로 본체만 잰다.
			const FShapedGlyphFontAtlasData AtlasData =
				FontCache->GetShapedGlyphFontAtlasData(Glyph, FFontOutlineSettings());
			if (!AtlasData.Valid)
			{
				continue;
			}

			const float BitmapRenderScale = Glyph.GetBitmapRenderScale();
			if (FMath::IsNearlyZero(BitmapRenderScale))
			{
				continue;
			}

			const float GlyphTop = -static_cast<float>(AtlasData.VerticalOffset)
				+ static_cast<float>(Glyph.YOffset)
				+ (MaxTextHeight + TextBaseline) / BitmapRenderScale;
			const float GlyphBottom = GlyphTop
				+ static_cast<float>(AtlasData.VSize) * BitmapRenderScale;
			InkTop = FMath::Min(InkTop, GlyphTop);
			InkBottom = FMath::Max(InkBottom, GlyphBottom);
			bFoundVisibleGlyph = true;
		}

		if (!bFoundVisibleGlyph)
		{
			return {};
		}

		// STextBlock의 실제 줄 박스는 본체 줄 높이 위아래에 OutlineSize를 더한다.
		const float LineBoxCenter = static_cast<float>(FontInfo.OutlineSettings.OutlineSize)
			+ MaxTextHeight * 0.5f;
		const float InkCenter = (InkTop + InkBottom) * 0.5f;
		return LineBoxCenter - InkCenter;
	}

	bool Apply(UTextBlock* TextBlock, const float FontStyleBiasY)
	{
		if (TextBlock == nullptr)
		{
			return false;
		}

		UOverlaySlot* CenterSlot = nullptr;
		for (UWidget* Node = TextBlock; Node != nullptr; Node = Node->GetParent())
		{
			CenterSlot = Cast<UOverlaySlot>(Node->Slot);
			if (CenterSlot != nullptr)
			{
				break;
			}
		}
		if (CenterSlot == nullptr)
		{
			return false;
		}

		const TOptional<float> OffsetY = MeasureOffsetY(*TextBlock);
		if (!OffsetY.IsSet())
		{
			return false;
		}

		const FMargin Padding = CenterSlot->GetPadding();
		CenterSlot->SetPadding(FMargin(Padding.Left, 0.0f, Padding.Right,
			-2.0f * (OffsetY.GetValue() + FontStyleBiasY)));
		return true;
	}
}
