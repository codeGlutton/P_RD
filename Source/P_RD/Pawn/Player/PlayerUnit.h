/*****************************************************************//**
 * @file   PlayerUnit.h
 * @brief  플레이어 베이스 유닛 정의 헤더 
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/Unit.h"
#include "PlayerUnit.generated.h"

/**
 * @brief  플레이어 베이스 유닛
 */
UCLASS(abstract)
class P_RD_API APlayerUnit : public AUnit
{
	GENERATED_BODY()
};
