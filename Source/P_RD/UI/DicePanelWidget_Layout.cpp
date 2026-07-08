#include "UI/DicePanelWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UI/DicePanelLayoutPolicy.h"
#include "UI/UIRuntimeLayout.h"



void UDicePanelWidget::ApplyDicePanelLayout()
{
	const int32 DiceCount = FMath::Min(mDicePanelViews.Num(), DicePanelSlotCount);
	if (DiceCount <= 0)
	{
		return;
	}

	if (mSelectedDicePanelIndex == INDEX_NONE || mSelectedDicePanelIndex >= DiceCount)
	{
		mSelectedDicePanelIndex = 0;
	}

	for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
	{
		if (mDicePanelCardBorders.IsValidIndex(DiceIndex) && mDicePanelCardBorders[DiceIndex] != nullptr)
		{
			mDicePanelCardBorders[DiceIndex]->SetVisibility(DiceIndex < DiceCount ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (mDicePanelCardTexture != nullptr)
			{
				mDicePanelCardBorders[DiceIndex]->SetBrushFromTexture(mDicePanelCardTexture);
			}
			mDicePanelCardBorders[DiceIndex]->SetBrushColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.96f));
		}
		if (mDicePanelImages.IsValidIndex(DiceIndex) && mDicePanelImages[DiceIndex] != nullptr)
		{
			mDicePanelImages[DiceIndex]->SetVisibility(DiceIndex < DiceCount ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	if (mDicePanelCarouselActivated == false)
	{
		TArray<FDicePanelSlotLayout> SlotLayouts;
		RDDicePanelLayout::BuildOverviewLayouts(DiceCount, OUT SlotLayouts);
		for (int32 DiceIndex = 0; DiceIndex < SlotLayouts.Num(); ++DiceIndex)
		{
			const FDicePanelSlotLayout& SlotLayout = SlotLayouts[DiceIndex];
			RDUILayout::ApplyAnchoredSlot(mDicePanelCardBorders[DiceIndex], SlotLayout.mCardAnchors, SlotLayout.mCardZOrder);
			RDUILayout::ApplyAnchoredSlot(mDicePanelImages[DiceIndex], SlotLayout.mImageAnchors, SlotLayout.mImageZOrder);
		}
		return;
	}

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		FDicePanelSlotLayout SlotLayout;
		if (RDDicePanelLayout::BuildCarouselLayout(DiceIndex, mSelectedDicePanelIndex, DiceCount, OUT SlotLayout) == false)
		{
			continue;
		}

		if (SlotLayout.mShowCard == false)
		{
			if (mDicePanelCardBorders.IsValidIndex(DiceIndex) && mDicePanelCardBorders[DiceIndex] != nullptr)
			{
				mDicePanelCardBorders[DiceIndex]->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (mDicePanelImages.IsValidIndex(DiceIndex) && mDicePanelImages[DiceIndex] != nullptr)
			{
				mDicePanelImages[DiceIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
				RDUILayout::ApplyAnchoredSlot(mDicePanelImages[DiceIndex], SlotLayout.mImageAnchors, SlotLayout.mImageZOrder);
			}
			continue;
		}

		if (mDicePanelCardBorders.IsValidIndex(DiceIndex) && mDicePanelCardBorders[DiceIndex] != nullptr)
		{
			mDicePanelCardBorders[DiceIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
			RDUILayout::ApplyAnchoredSlot(mDicePanelCardBorders[DiceIndex], SlotLayout.mCardAnchors, SlotLayout.mCardZOrder);
		}
		if (mDicePanelImages.IsValidIndex(DiceIndex) && mDicePanelImages[DiceIndex] != nullptr)
		{
			mDicePanelImages[DiceIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
			RDUILayout::ApplyAnchoredSlot(mDicePanelImages[DiceIndex], SlotLayout.mImageAnchors, SlotLayout.mImageZOrder);
		}
	}
}
