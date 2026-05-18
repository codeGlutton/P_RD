/*****************************************************************//**
 * @file   EquipmentType.h
 * @brief  장비 타입 정의 헤더
 * @author 모호재
 * @date   2026-05-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "EquipmentType.generated.h"

/**
 * @brief  장비 타입
 */
UENUM(BlueprintType)
enum class EEquipmentType : uint8
{
	Weapon = 0,
	Gloves,
	Boots,
	Count		UMETA(Hidden)
};
