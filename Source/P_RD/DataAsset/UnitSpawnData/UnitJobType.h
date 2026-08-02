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
	/* 확정 직업 */ 

	Knight = 0,

	/* 아래는 수정될 수 있는 임시 직업 */

	Archer,
	Mage,

	PlayerJobCount			UMETA(Hidden),
	
	/* 추가 열거형 */

	Common,
	Monster,

	None = 0xFF		UMETA(Hidden),
};
