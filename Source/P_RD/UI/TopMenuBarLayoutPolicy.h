#pragma once

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

namespace RDTopMenuBarLayout
{
	/** @brief 런타임 투명 hit area 디버그/터치용 색. 실제 배경을 가리지 않도록 알파가 낮다. */
	P_RD_API FLinearColor GetRuntimeHitAreaColor();
	/** @brief DICE 버튼의 시각 위치보다 아래로 확장한 모바일 터치 anchor. */
	P_RD_API FAnchors GetDiceHitAreaAnchors();
	/** @brief SKILL 버튼의 시각 위치보다 아래로 확장한 모바일 터치 anchor. */
	P_RD_API FAnchors GetSkillHitAreaAnchors();
	/** @brief 디자이너 버튼 위에 얹히는 투명 hit area ZOrder. */
	P_RD_API int32 GetRuntimeHitAreaZOrder();
}
