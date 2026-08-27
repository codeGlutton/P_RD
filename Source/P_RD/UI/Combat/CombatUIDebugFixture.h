#pragma once

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"

namespace CombatUIDebugFixture
{
	/** @brief 0=off, 1=display-only HP one, 2=status matrix, 3=both. */
	P_RD_API int32 GetMode();

	/** @brief Legacy opt-in only. Mode 1/3 must never mutate gameplay attributes. */
	P_RD_API bool ShouldMutateActualHPOne();

	/** @brief Applies the HP-one fixture only to a live unit's UI DTO. Max HP is untouched. */
	P_RD_API void ApplyDisplayHPOne(bool bUnitAlive, OUT FUnitUI& UnitUIData);

	P_RD_API void AppendStatuses(bool bPlayer, int32 SideIndex,
		OUT TArray<FStatusEffectUI>& Statuses);
}
