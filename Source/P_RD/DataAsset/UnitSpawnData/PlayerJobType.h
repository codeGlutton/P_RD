/*****************************************************************//**
 * @file   PlayerJobType.h
 * @brief  플레이어 직업 열거형 정의 헤더
 * @author 모호재
 * @date   2026-06-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PlayerJobType.generated.h"

/**
 * @brief  플레이어 직업 열거형
 */
UENUM(BlueprintType)
enum class EPlayerJobType : uint8
{
	/* 확정 직업 */ 

	Knight = 0,

	/* 아래는 수정될 수 있는 임시 직업 */

	Archer,
	Mage,

	Count			UMETA(Hidden),
	
	/* 스킬 클래스 타입 구분을 위한 추가 열거형 */

	Common,
	Monster,

	None = 0xFF		UMETA(Hidden),
};
