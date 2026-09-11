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
	Attack = 0	UMETA(ToolTip = "AI가 적을 타겟팅할 필요가 있는 경우"),
	Spell		UMETA(ToolTip = "AI가 스스로를 타겟팅할 필요가 있는 경우"),
	Move		UMETA(ToolTip = "AI가 빈 공간을 타겟팅할 필요가 있는 경우"),
	Count		UMETA(Hidden)
};
