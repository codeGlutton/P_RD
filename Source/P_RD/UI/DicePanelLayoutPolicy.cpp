#include "UI/DicePanelLayoutPolicy.h"

#include "UI/DiceCapturePreviewUtils.h"

int32 RDDicePanelLayout::GetPreviewRenderTargetSize()
{
	return RDDiceCapturePreview::GetDefaultRenderTargetSize();
}

FVector2D RDDicePanelLayout::GetPreviewBrushSize()
{
	const float PreviewSize = StaticCast<float>(GetPreviewRenderTargetSize());
	return FVector2D(PreviewSize, PreviewSize);
}

FVector RDDicePanelLayout::GetPreviewActorLocation(int32 DiceIndex)
{
	return FVector(0.0f, 42000.0f + StaticCast<float>(DiceIndex) * 420.0f, 30000.0f);
}

float RDDicePanelLayout::GetPreviewDiceScale(bool bSelected)
{
	return bSelected ? 1.28f : 1.04f;
}

FRotator RDDicePanelLayout::GetIdleRotation(int32 DiceIndex)
{
	static const FRotator IdleRotations[] =
	{
		FRotator(24.0f, -38.0f, 16.0f),
		FRotator(18.0f, -20.0f, -10.0f),
		FRotator(28.0f, -52.0f, 8.0f),
		FRotator(16.0f, -30.0f, 18.0f),
		FRotator(22.0f, -44.0f, -14.0f),
		FRotator(14.0f, -28.0f, 20.0f),
	};

	return IdleRotations[DiceIndex % UE_ARRAY_COUNT(IdleRotations)];
}

void RDDicePanelLayout::BuildOverviewLayouts(int32 DiceCount, TArray<FDicePanelSlotLayout>& OutLayouts)
{
	OutLayouts.Reset();
	if (DiceCount <= 0)
	{
		return;
	}

	const float CardWidth = 0.140f;
	const float CardHeight = 0.270f;
	const float Gap = 0.020f;
	const float TotalWidth = CardWidth * StaticCast<float>(DiceCount) + Gap * StaticCast<float>(FMath::Max(0, DiceCount - 1));
	const float StartLeft = 0.5f - TotalWidth * 0.5f;
	const float Top = 0.445f;

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		const float Left = StartLeft + (CardWidth + Gap) * StaticCast<float>(DiceIndex);

		FDicePanelSlotLayout Layout;
		Layout.mCardAnchors = FAnchors(Left, Top, Left + CardWidth, Top + CardHeight);
		Layout.mImageAnchors = Layout.mCardAnchors;
		Layout.mCardZOrder = 126 + DiceIndex;
		Layout.mImageZOrder = 127 + DiceIndex;
		Layout.mShowCard = true;
		OutLayouts.Add(Layout);
	}
}

bool RDDicePanelLayout::BuildCarouselLayout(int32 DiceIndex, int32 SelectedDiceIndex, int32 DiceCount, FDicePanelSlotLayout& OutLayout)
{
	if (DiceIndex < 0 || DiceIndex >= DiceCount || SelectedDiceIndex < 0 || SelectedDiceIndex >= DiceCount)
	{
		return false;
	}

	if (DiceIndex == SelectedDiceIndex)
	{
		OutLayout.mCardAnchors = FAnchors();
		OutLayout.mImageAnchors = FAnchors(0.391f, 0.365f, 0.609f, 0.715f);
		OutLayout.mCardZOrder = 0;
		OutLayout.mImageZOrder = 170;
		OutLayout.mShowCard = false;
		return true;
	}

	int32 RelativeIndex = DiceIndex - SelectedDiceIndex;
	if (RelativeIndex > DiceCount / 2)
	{
		RelativeIndex -= DiceCount;
	}
	else if (RelativeIndex < -DiceCount / 2)
	{
		RelativeIndex += DiceCount;
	}

	const float Direction = RelativeIndex < 0 ? -1.0f : 1.0f;
	const float Distance = StaticCast<float>(FMath::Max(1, FMath::Abs(RelativeIndex)));
	const float CenterX = 0.5f + Direction * (0.265f + (Distance - 1.0f) * 0.120f);
	const float CardWidth = 0.120f;
	const float CardHeight = 0.235f;
	const float Top = 0.475f + FMath::Min(Distance - 1.0f, 2.0f) * 0.030f;
	const int32 ZOrder = 150 - FMath::RoundToInt(Distance * 4.0f);

	OutLayout.mCardAnchors = FAnchors(CenterX - CardWidth * 0.5f, Top, CenterX + CardWidth * 0.5f, Top + CardHeight);
	OutLayout.mImageAnchors = OutLayout.mCardAnchors;
	OutLayout.mCardZOrder = ZOrder;
	OutLayout.mImageZOrder = ZOrder + 1;
	OutLayout.mShowCard = true;
	return true;
}
