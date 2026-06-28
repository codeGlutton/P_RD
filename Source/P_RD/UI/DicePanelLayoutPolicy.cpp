#include "UI/DicePanelLayoutPolicy.h"

#include "UI/DiceCapturePreviewUtils.h"

/** @brief 패널용 캡처 RT 크기는 공용 캡처 기본값을 따른다. */
int32 RDDicePanelLayout::GetPreviewRenderTargetSize()
{
	return RDDiceCapturePreview::GetDefaultRenderTargetSize();
}

/** @brief RT 크기와 같은 정사각 brush 크기를 만든다. */
FVector2D RDDicePanelLayout::GetPreviewBrushSize()
{
	const float PreviewSize = StaticCast<float>(GetPreviewRenderTargetSize());
	return FVector2D(PreviewSize, PreviewSize);
}

/** @brief DiceIndex별 캡처 액터를 전투 월드와 겹치지 않는 원거리 슬롯에 배치한다. */
FVector RDDicePanelLayout::GetPreviewActorLocation(int32 DiceIndex)
{
	// 42000대 Y 좌표는 HUD 인트로 캡처 영역과도 분리된 패널 전용 격리 영역이다.
	return FVector(0.0f, 42000.0f + StaticCast<float>(DiceIndex) * 420.0f, 30000.0f);
}

/** @brief Carousel 선택 주사위는 더 크게 캡처해 상세 모드의 중심 카드 밀도를 맞춘다. */
float RDDicePanelLayout::GetPreviewDiceScale(bool bSelected)
{
	return bSelected ? 1.34f : 1.10f;
}

/** @brief 슬롯별 기본 회전을 달리해 overview에서 모든 주사위가 같은 면만 보이지 않게 한다. */
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

/** @brief 보유 주사위 수를 화면 중앙에 균등 배치하는 overview layout을 만든다. */
void RDDicePanelLayout::BuildOverviewLayouts(int32 DiceCount, TArray<FDicePanelSlotLayout>& OutLayouts)
{
	OutLayouts.Reset();
	if (DiceCount <= 0)
	{
		return;
	}

	const int32 ColumnCount = FMath::Min(3, DiceCount);
	const int32 RowCount = FMath::CeilToInt(StaticCast<float>(DiceCount) / StaticCast<float>(ColumnCount));
	const float CardWidth = 0.135f;
	const float CardHeight = 0.235f;
	const float GapX = 0.040f;
	const float GapY = 0.040f;
	const float TotalWidth = CardWidth * StaticCast<float>(ColumnCount) + GapX * StaticCast<float>(FMath::Max(0, ColumnCount - 1));
	const float TotalHeight = CardHeight * StaticCast<float>(RowCount) + GapY * StaticCast<float>(FMath::Max(0, RowCount - 1));
	const float StartLeft = 0.5f - TotalWidth * 0.5f;
	const float StartTop = 0.5f - TotalHeight * 0.5f + 0.025f;

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		const int32 Column = DiceIndex % ColumnCount;
		const int32 Row = DiceIndex / ColumnCount;
		const float Left = StartLeft + (CardWidth + GapX) * StaticCast<float>(Column);
		const float Top = StartTop + (CardHeight + GapY) * StaticCast<float>(Row);

		FDicePanelSlotLayout Layout;
		Layout.mCardAnchors = FAnchors(Left, Top, Left + CardWidth, Top + CardHeight);
		Layout.mImageAnchors = Layout.mCardAnchors;
		Layout.mCardZOrder = 126 + DiceIndex;
		Layout.mImageZOrder = 127 + DiceIndex;
		Layout.mShowCard = true;
		OutLayouts.Add(Layout);
	}
}

/** @brief 선택 주사위는 중앙 크게, 나머지는 원형 carousel의 좌우 후보처럼 배치한다. */
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

	// 원형 carousel이라 끝과 처음이 인접하도록 상대 index를 가장 짧은 방향으로 접는다.
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
