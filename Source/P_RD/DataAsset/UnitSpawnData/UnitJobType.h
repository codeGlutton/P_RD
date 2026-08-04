/*****************************************************************//**
 * @file   UnitJobType.h
 * @brief  직업 열거형 정의 헤더
 * @author 모호재
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UnitJobType.generated.h"

/**
 * @brief  직업 열거형
 */
UENUM(BlueprintType)
enum class EUnitJobType : uint8
{
	/* 용병 직업 */ 

	Knight = 0,
	Ranger,
	Mage,
	Barbarian,
	Rogue,
	Druid,
	Engineer,

	PlayerJobCount			UMETA(Hidden),
	
	/* 추가 열거형 */

	Common,
	Monster,

	None = 0xFF		UMETA(Hidden),
};
