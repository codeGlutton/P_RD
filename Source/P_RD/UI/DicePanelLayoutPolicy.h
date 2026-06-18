#pragma once

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

struct P_RD_API FDicePanelSlotLayout
{
	/** @brief 카드 배경의 Canvas anchor. Overview/Carousel 모드 모두 화면 비율 기준이다. */
	FAnchors mCardAnchors;

	/** @brief RenderTarget Image anchor. 카드보다 살짝 앞/안쪽에 배치된다. */
	FAnchors mImageAnchors;

	/** @brief 카드 배경 ZOrder. Image보다 낮아야 주사위가 가려지지 않는다. */
	int32 mCardZOrder = 0;

	/** @brief 주사위 RenderTarget Image ZOrder. */
	int32 mImageZOrder = 0;

	/** @brief Overview에서 실제 보유 주사위보다 많은 WBP 슬롯을 숨기기 위한 플래그. */
	bool mShowCard = true;
};

namespace RDDicePanelLayout
{
	/** @brief 주사위 패널 캡처 RenderTarget 크기. HUD 인트로보다 패널 상세 가독성을 우선한다. */
	P_RD_API int32 GetPreviewRenderTargetSize();
	/** @brief RenderTarget 크기와 같은 정사각 brush 크기. */
	P_RD_API FVector2D GetPreviewBrushSize();
	/** @brief 패널 전용 캡처 액터를 게임 월드와 충돌하지 않는 원거리 슬롯에 배치한다. */
	P_RD_API FVector GetPreviewActorLocation(int32 DiceIndex);
	/** @brief 선택/비선택 상태에 따른 캡처 주사위 스케일 보정값. */
	P_RD_API float GetPreviewDiceScale(bool bSelected);
	/** @brief Overview 카드별 기본 idle 회전. 같은 각도 반복으로 보이는 것을 피한다. */
	P_RD_API FRotator GetIdleRotation(int32 DiceIndex);
	/** @brief 보유 주사위 수에 맞춰 Overview 카드/Image 레이아웃을 만든다. */
	P_RD_API void BuildOverviewLayouts(int32 DiceCount, TArray<FDicePanelSlotLayout>& OutLayouts);
	/** @brief Carousel 모드에서 선택 카드와 주변 카드의 레이아웃을 만든다. */
	P_RD_API bool BuildCarouselLayout(int32 DiceIndex, int32 SelectedDiceIndex, int32 DiceCount, FDicePanelSlotLayout& OutLayout);
}
