#pragma once

#include "CoreMinimal.h"
#include "UnitCombatCondition.generated.h"

UENUM(BlueprintType)
enum class EUnitCombatCondition : uint8
{
	Bad = 0 UMETA(ToolTip = "최소 데미지 확정"),
	Normal UMETA(ToolTip = "평균 데미지 확정"),
	Good UMETA(ToolTip = "최대 데미지 확정"),
	Excellent UMETA(ToolTip = "최대 데미지와 치명타 확정"),
	Count UMETA(Hidden),
};
