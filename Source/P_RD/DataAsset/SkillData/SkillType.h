/*****************************************************************//**
 * @file   SkillType.h
 * @brief  스킬 타입 정의 헤더
 * @author 모호재
 * @date   2026-05-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SkillType.generated.h"

/**
 * @brief  스킬 타입
 */
UENUM(BlueprintType)
enum class ESkillType : uint8
{
	Attack = 0,
	Spell,
	Count		UMETA(Hidden)
};
