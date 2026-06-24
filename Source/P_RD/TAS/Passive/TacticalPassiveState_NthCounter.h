/*****************************************************************//**
 * @file   TacticalPassiveState_NthCounter.h
 * @brief  N번째 발동 카운터 패시브 상태
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "TAS/Passive/TacticalPassiveState.h"
#include "TacticalPassiveState_NthCounter.generated.h"

/**
 * @brief N번째 발동 카운터 상태
 *
 * @details
 * 매 발동마다 증가하는 카운터를 보관.
 * 임계값(N) 도달 시 발동 + 리셋하는 Nth 계열 패시브(NthAddAttack, NthMultiplyAttack 등)가 공유.
 * 임계값(N)은 변하지 않는 config라 패시브 클래스에 두고, 여기에는 변하는 카운터만 둠.
 */
USTRUCT()
struct FTacticalPassiveState_NthCounter : public FTacticalPassiveState
{
	GENERATED_BODY()

	// 현재 누적 카운트 (임계값 도달 시 0으로 리셋)
	UPROPERTY()
	int32 mCount = 0;
};
