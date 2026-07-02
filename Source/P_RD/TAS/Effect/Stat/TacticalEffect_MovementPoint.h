/*****************************************************************//**
 * @file   TacticalEffect_MovementPoint.h
 * @brief  MovementPoint 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_MovementPoint.generated.h"

/**
 * @brief MovementPoint 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_MovementPoint : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_MovementPoint();
};
