/*****************************************************************//**
 * @file   StageLevelType.h
 * @brief  스테이지 레벨 타입 구현 헤더
 * @author 모호재
 * @date   2026-05-13
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "StageLevelType.generated.h"

/**
 * @brief  스테이지 레벨 타입 (실수 방지)
 */
UENUM(BlueprintType)
enum class EStageLevelType : uint8
{
    None = 0    UMETA(Hidden),
    Stage1,
    Stage2,
    Stage3,
};

