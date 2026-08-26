#pragma once

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"

namespace CombatUIDebugFixture
{
	/** @brief 0=off, 1=display-only HP one, 2=status matrix, 3=both. */
	P_RD_API int32 GetMode();

	/** @brief Development-only all-unit current-HP=1 fixture. Shipping is always off. */
	P_RD_API bool ShouldMutateActualHPOne();

	/**
	 * @brief 아군 용병까지 실제 HP 1로 만들지 여부.
	 *
	 * @details ``rd.Debug.ForceCombatHPOne 2`` 에서만 참이다. 1은 예전처럼
	 * 적만 바꾼다 -- 아군 HP는 RunPersistData로 방 밖까지 따라가므로, 켜는
	 * 사람이 그 사실을 알고 골라야 한다(0824 요청: "용병이랑 적 모두 HP 1").
	 */
	P_RD_API bool ShouldMutatePlayerHPOne();

	/** @brief Applies the HP-one fixture only to a live unit's UI DTO. Max HP is untouched. */
	P_RD_API void ApplyDisplayHPOne(bool bUnitAlive, OUT FUnitUI& UnitUIData);

	P_RD_API void AppendStatuses(bool bPlayer, int32 SideIndex,
		OUT TArray<FStatusEffectUI>& Statuses);
}
