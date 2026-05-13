/*****************************************************************//**
 * @file   RarityType.h
 * @brief  희귀도 열거형 정의 헤더
 * @author 모호재
 * @date   2026-05-13
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "RarityType.generated.h"

/**
 * @brief  희귀도 열거형
 */
UENUM(BlueprintType)
enum class ERarityType : uint8
{
	Common,
	Rare,
	Epic,
};

