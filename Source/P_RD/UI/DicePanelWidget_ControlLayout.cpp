#include "UI/DicePanelWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/IndexedButtonWidget.h"
#include "UI/UIRuntimeLayout.h"

void UDicePanelWidget::ApplyDiceCarouselControlLayout() const
{
	RDUILayout::ApplyAnchoredSlot(mDiceCarouselTitleText, FAnchors(0.355f, 0.345f, 0.645f, 0.395f), 210);
	RDUILayout::ApplyAnchoredSlot(mDiceCarouselFaceText, FAnchors(0.330f, 0.855f, 0.670f, 0.895f), 210);

	const int32 FaceButtonCount = mDicePanelViews.IsValidIndex(mSelectedDicePanelIndex)
		? FMath::Clamp(mDicePanelViews[mSelectedDicePanelIndex].mFaceCount, 0, DicePanelMaxFaceButtonCount)
		: 0;
	const float FaceButtonWidth = FaceButtonCount > 12 ? 0.038f : 0.052f;
	const float FaceButtonGap = FaceButtonCount > 12 ? 0.006f : 0.012f;
	const float FaceButtonTotalWidth = FaceButtonCount > 0
		? FaceButtonWidth * StaticCast<float>(FaceButtonCount) + FaceButtonGap * StaticCast<float>(FaceButtonCount - 1)
		: 0.0f;
	const float FaceButtonStartLeft = 0.5f - FaceButtonTotalWidth * 0.5f;
	for (int32 FaceIndex = 0; FaceIndex < mDiceCarouselFaceButtons.Num(); ++FaceIndex)
	{
		const float Left = FaceButtonStartLeft + (FaceButtonWidth + FaceButtonGap) * StaticCast<float>(FaceIndex);
		RDUILayout::ApplyAnchoredSlot(mDiceCarouselFaceButtons[FaceIndex].Get(), FAnchors(Left, 0.795f, Left + FaceButtonWidth, 0.850f), 211);
	}

	RDUILayout::ApplyAnchoredSlot(mDiceCarouselPreviousButton, FAnchors(0.065f, 0.535f, 0.125f, 0.650f), 211);
	RDUILayout::ApplyAnchoredSlot(mDiceCarouselNextButton, FAnchors(0.875f, 0.535f, 0.935f, 0.650f), 211);
	RDUILayout::ApplyAnchoredSlot(mDiceCarouselBackButton, FAnchors(0.405f, 0.910f, 0.595f, 0.970f), 211);
}
