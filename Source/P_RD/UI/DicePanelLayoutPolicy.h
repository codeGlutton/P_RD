#pragma once

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

struct P_RD_API FDicePanelSlotLayout
{
	FAnchors mCardAnchors;
	FAnchors mImageAnchors;
	int32 mCardZOrder = 0;
	int32 mImageZOrder = 0;
	bool mShowCard = true;
};

namespace RDDicePanelLayout
{
	P_RD_API int32 GetPreviewRenderTargetSize();
	P_RD_API FVector2D GetPreviewBrushSize();
	P_RD_API FVector GetPreviewActorLocation(int32 DiceIndex);
	P_RD_API float GetPreviewDiceScale(bool bSelected);
	P_RD_API FRotator GetIdleRotation(int32 DiceIndex);
	P_RD_API void BuildOverviewLayouts(int32 DiceCount, TArray<FDicePanelSlotLayout>& OutLayouts);
	P_RD_API bool BuildCarouselLayout(int32 DiceIndex, int32 SelectedDiceIndex, int32 DiceCount, FDicePanelSlotLayout& OutLayout);
}
