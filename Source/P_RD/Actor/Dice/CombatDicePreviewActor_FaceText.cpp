#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/TextRenderComponent.h"
#include "Actor/Dice/CombatDicePreviewActorPrivate.h"
#include "Actor/Dice/DicePolyhedron.h"

namespace
{
	constexpr float FaceTextLift = 0.060f;
	constexpr float FaceTextStrokeLift = 0.058f;
	constexpr float FaceTextStrokeOffset = 0.0065f;
	constexpr float CoinFaceTextLift = 0.185f;
	constexpr float CoinFaceTextStrokeLift = 0.178f;
	constexpr float CoinFaceTextStrokeOffset = 0.0105f;
	constexpr int32 FaceTextStrokeCount = 4;
	const FColor FaceTextDefaultColor(24, 24, 30, 230);
	const FColor FaceTextDefaultStrokeColor(12, 12, 18, 190);
	const FColor FaceTextDimColor(132, 132, 144, 128);
	const FColor FaceTextDimStrokeColor(120, 120, 132, 80);
	const FColor FaceTextHighlightColor(4, 4, 8, 255);
	const FColor FaceTextHighlightStrokeColor(0, 0, 0, 240);
	constexpr float MinFaceTextScale = 0.65f;
	constexpr float MaxFaceTextScale = 2.20f;

	FVector MakeFaceRightVector(const RDDicePolyhedron::FDiceFace& Face)
	{
		FVector Right = FVector::CrossProduct(Face.mUp, Face.mNormal).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			Right = FVector::CrossProduct(FVector::YAxisVector, Face.mNormal).GetSafeNormal();
		}
		return Right;
	}
}

void ACombatDicePreviewActor::RebuildFaceTexts(int32 FaceCount)
{
	// 기존 TextRender 제거.
	for (UTextRenderComponent* FaceText : mFaceTexts)
	{
		if (FaceText != nullptr)
		{
			FaceText->DestroyComponent();
		}
	}
	mFaceTexts.Reset();
	for (UTextRenderComponent* FaceTextStroke : mFaceTextStrokes)
	{
		if (FaceTextStroke != nullptr)
		{
			FaceTextStroke->DestroyComponent();
		}
	}
	mFaceTextStrokes.Reset();
	const RDDicePolyhedron::FDicePolyhedron& Poly = RDDicePolyhedron::Get(FaceCount);

	// 주사위 종류당 숫자 크기를 하나로 통일(면마다 들쭉날쭉하지 않게). 가장 작은 면에도 들어가게 최소 내접반지름 기준.
	float MinInradius = TNumericLimits<float>::Max();
	for (int32 ValueFaceIndex = 0; ValueFaceIndex < Poly.GetValueFaceCount(); ++ValueFaceIndex)
	{
		const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(ValueFaceIndex);
		const int32 EdgeCount = Face.mVertexIndices.Num();
		for (int32 Edge = 0; Edge < EdgeCount; ++Edge)
		{
			const FVector& V0 = Poly.mVertices[Face.mVertexIndices[Edge]];
			const FVector& V1 = Poly.mVertices[Face.mVertexIndices[(Edge + 1) % EdgeCount]];
			MinInradius = FMath::Min(MinInradius, static_cast<float>(((V0 + V1) * 0.5f - Face.mCenter).Size()));
		}
	}
	// 주사위 간에도 너무 차이나지 않게 클램프.
	mBaseFaceTextWorldSize = FMath::Clamp(MinInradius * DiceRadius * 1.48f, 24.0f, 48.0f);
	if (FaceCount == 2)
	{
		mBaseFaceTextWorldSize = 44.0f;
	}
	const float UniformTextSize = mBaseFaceTextWorldSize * mFaceTextScale;

	for (int32 FaceIndex = 0; FaceIndex < Poly.GetValueFaceCount(); ++FaceIndex)
	{
		const RDDicePolyhedron::FDiceFace& Face = Poly.GetValueFace(FaceIndex);
		const float TextWorldSize = UniformTextSize;
		const float TextLift = FaceCount == 2 ? CoinFaceTextLift : FaceTextLift;
		const float StrokeLift = FaceCount == 2 ? CoinFaceTextStrokeLift : FaceTextStrokeLift;
		const float CurrentStrokeOffset = FaceCount == 2 ? CoinFaceTextStrokeOffset : FaceTextStrokeOffset;

		const FVector BaseTextLocation = Face.mCenter * DiceRadius + Face.mNormal * (DiceRadius * TextLift);
		const FVector StrokeBaseLocation = Face.mCenter * DiceRadius + Face.mNormal * (DiceRadius * StrokeLift);
		const FVector Right = MakeFaceRightVector(Face);
		const float StrokeOffset = DiceRadius * CurrentStrokeOffset;
		const FRotator TextRotation = RDCombatDicePreview::MakeFaceTextRotation(Face.mNormal, Face.mUp);
		const FText DefaultText = FText::AsNumber(FaceIndex + 1);
		const FVector StrokeDirections[FaceTextStrokeCount] =
		{
			Right,
			-Right,
			Face.mUp,
			-Face.mUp
		};

		for (int32 StrokeIndex = 0; StrokeIndex < FaceTextStrokeCount; ++StrokeIndex)
		{
			UTextRenderComponent* FaceTextStroke = NewObject<UTextRenderComponent>(this);
			if (FaceTextStroke == nullptr)
			{
				mFaceTextStrokes.Add(nullptr);
				continue;
			}

			FaceTextStroke->SetupAttachment(mDiceRoot);
			FaceTextStroke->RegisterComponent();
			FaceTextStroke->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
			FaceTextStroke->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
			FaceTextStroke->SetTextRenderColor(FaceTextDefaultStrokeColor);
			FaceTextStroke->SetWorldSize(TextWorldSize);
			FaceTextStroke->SetCastShadow(false);
			FaceTextStroke->SetTranslucentSortPriority(FaceCount == 2 ? 64 : 8);
			if (mDiceNumberFont != nullptr)
			{
				FaceTextStroke->SetFont(mDiceNumberFont);
			}
			FaceTextStroke->SetRelativeLocation(StrokeBaseLocation + StrokeDirections[StrokeIndex] * StrokeOffset);
			FaceTextStroke->SetRelativeRotation(TextRotation);
			FaceTextStroke->SetText(DefaultText);
			mFaceTextStrokes.Add(FaceTextStroke);
		}

		UTextRenderComponent* FaceText = NewObject<UTextRenderComponent>(this);
		if (FaceText == nullptr)
		{
			mFaceTexts.Add(nullptr);
			continue;
		}
		FaceText->SetupAttachment(mDiceRoot);
		FaceText->RegisterComponent();
		FaceText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		FaceText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		FaceText->SetTextRenderColor(FaceTextDefaultColor);   // 밝은 주사위에 대비되는 진한 숫자
		FaceText->SetWorldSize(TextWorldSize);
		FaceText->SetCastShadow(false);
		FaceText->SetTranslucentSortPriority(FaceCount == 2 ? 65 : 9);
		if (mDiceNumberFont != nullptr)
		{
			FaceText->SetFont(mDiceNumberFont);
		}

		// 면 중심에서 바깥으로 띄워, 숫자가 주사위 몸체에 가리지 않게.
		FaceText->SetRelativeLocation(BaseTextLocation);
		FaceText->SetRelativeRotation(TextRotation);
		FaceText->SetText(DefaultText);

		mFaceTexts.Add(FaceText);
	}

	ApplyFaceTextStyles();
}

void ACombatDicePreviewActor::SetFaceText(int32 FaceIndex, const FText& FaceText)
{
	const int32 ArrayIndex = FaceIndex - 1;
	if (mFaceTexts.IsValidIndex(ArrayIndex) && mFaceTexts[ArrayIndex] != nullptr)
	{
		mFaceTexts[ArrayIndex]->SetText(FaceText);
	}

	const int32 StrokeStartIndex = ArrayIndex * FaceTextStrokeCount;
	for (int32 StrokeOffsetIndex = 0; StrokeOffsetIndex < FaceTextStrokeCount; ++StrokeOffsetIndex)
	{
		const int32 StrokeIndex = StrokeStartIndex + StrokeOffsetIndex;
		if (mFaceTextStrokes.IsValidIndex(StrokeIndex) && mFaceTextStrokes[StrokeIndex] != nullptr)
		{
			mFaceTextStrokes[StrokeIndex]->SetText(FaceText);
		}
	}
}

void ACombatDicePreviewActor::SetHighlightedFace(int32 FaceOrdinal)
{
	mHighlightedFaceOrdinal = FaceOrdinal > 0 ? FaceOrdinal : INDEX_NONE;
	ApplyFaceTextStyles();
}

void ACombatDicePreviewActor::ClearHighlightedFace()
{
	mHighlightedFaceOrdinal = INDEX_NONE;
	ApplyFaceTextStyles();
}

void ACombatDicePreviewActor::SetHideNonHighlightedFaceTexts(bool bHide)
{
	mHideNonHighlightedFaceTexts = bHide;
	ApplyFaceTextStyles();
}

void ACombatDicePreviewActor::SetFaceTextScale(float NewScale)
{
	mFaceTextScale = FMath::Clamp(NewScale, MinFaceTextScale, MaxFaceTextScale);
	ApplyFaceTextScale();
}

void ACombatDicePreviewActor::ApplyFaceTextScale()
{
	const float TextWorldSize = mBaseFaceTextWorldSize * mFaceTextScale;
	for (UTextRenderComponent* FaceText : mFaceTexts)
	{
		if (FaceText != nullptr)
		{
			FaceText->SetWorldSize(TextWorldSize);
		}
	}
	for (UTextRenderComponent* FaceTextStroke : mFaceTextStrokes)
	{
		if (FaceTextStroke != nullptr)
		{
			FaceTextStroke->SetWorldSize(TextWorldSize);
		}
	}
}

void ACombatDicePreviewActor::ApplyFaceTextStyles()
{
	const bool bHasHighlight = mHighlightedFaceOrdinal != INDEX_NONE;
	for (int32 FaceIndex = 0; FaceIndex < mFaceTexts.Num(); ++FaceIndex)
	{
		const bool bHighlighted = bHasHighlight && mHighlightedFaceOrdinal == FaceIndex + 1;
		const bool bHideDimText = mHideNonHighlightedFaceTexts && (bHasHighlight == false || bHighlighted == false);
		const FColor MainColor = bHasHighlight
			? (bHighlighted ? FaceTextHighlightColor : FaceTextDimColor)
			: FaceTextDefaultColor;
		const FColor StrokeColor = bHasHighlight
			? (bHighlighted ? FaceTextHighlightStrokeColor : FaceTextDimStrokeColor)
			: FaceTextDefaultStrokeColor;

		if (mFaceTexts[FaceIndex] != nullptr)
		{
			mFaceTexts[FaceIndex]->SetVisibility(bHideDimText == false, false);
			mFaceTexts[FaceIndex]->SetTextRenderColor(MainColor);
		}

		const bool bShowStroke = bHideDimText == false && (bHasHighlight == false || bHighlighted == true);
		const int32 StrokeStartIndex = FaceIndex * FaceTextStrokeCount;
		for (int32 StrokeOffsetIndex = 0; StrokeOffsetIndex < FaceTextStrokeCount; ++StrokeOffsetIndex)
		{
			const int32 StrokeIndex = StrokeStartIndex + StrokeOffsetIndex;
			if (mFaceTextStrokes.IsValidIndex(StrokeIndex) && mFaceTextStrokes[StrokeIndex] != nullptr)
			{
				mFaceTextStrokes[StrokeIndex]->SetVisibility(bShowStroke, false);
				mFaceTextStrokes[StrokeIndex]->SetTextRenderColor(StrokeColor);
			}
		}
	}
}
