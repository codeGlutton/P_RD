#pragma once

#include "RDMinimal.h"
#include "Components/CanvasPanelSlot.h"

namespace RDTopMenuBarLayout
{
	P_RD_API FLinearColor GetRuntimeHitAreaColor();
	P_RD_API FAnchors GetDiceHitAreaAnchors();
	P_RD_API FAnchors GetSkillHitAreaAnchors();
	P_RD_API int32 GetRuntimeHitAreaZOrder();
}
