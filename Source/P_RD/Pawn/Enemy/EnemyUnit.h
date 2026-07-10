/*****************************************************************//**
 * @file   EnemyUnit.h
 * @brief  적 베이스 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/Unit.h"
#include "EnemyUnit.generated.h"

/**
 * @brief  적 베이스 유닛
 */
UCLASS(abstract)
class P_RD_API AEnemyUnit : public AUnit
{
	GENERATED_BODY()
};
