#pragma once

#include "RDMinimal.h"

class UTextBlock;

/** 실제 글리프 잉크 경계를 기준으로 UMG 텍스트를 광학 중앙에 맞춘다. */
namespace RDTextOpticalAlignment
{
	/** 현재 문자열과 폰트의 줄 박스 중앙에서 실제 글리프 중앙까지의 Y 이동량. */
	P_RD_API TOptional<float> MeasureOffsetY(const UTextBlock& TextBlock);

	/** 가장 가까운 Overlay 슬롯에 측정값과 폰트 스타일 공통 보정값을 적용한다. */
	P_RD_API bool Apply(UTextBlock* TextBlock, float FontStyleBiasY = 0.0f);
}
