/*****************************************************************//**
 * @file   PlayerUnit.h
 * @brief  플레이어 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-11
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

#include "Pawn/Unit.h"

#include "PlayerUnit.generated.h"

/**
 * @brief  SRPG에서 사용되는 베이스 폰 클래스
 */
UCLASS()
class P_RD_API APlayerUnit : public AUnit
{
	GENERATED_BODY()
};
