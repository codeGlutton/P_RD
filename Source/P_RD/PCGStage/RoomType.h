/*****************************************************************//**
 * @file   RoomType.h
 * @brief  방 타입 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "RoomType.generated.h"

/**
 * @brief  방 타입
 */
UENUM(BlueprintType)
enum class ERoomType : uint8
{
	None = 255		UMETA(Hidden),
	Treasure = 0,
	Shop,
	Monster,
	EliteMonster,
	BossMonster,
	Count			UMETA(Hidden),
};

