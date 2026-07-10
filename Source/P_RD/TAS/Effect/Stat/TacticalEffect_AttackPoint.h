/*****************************************************************//**
 * @file   TacticalEffect_AttackPoint.h
 * @brief  AttackPoint 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-06-26
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_AttackPoint.generated.h"

/**
 * @brief AttackPoint 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AttackPoint : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackPoint();
};
