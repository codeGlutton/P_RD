/*****************************************************************//**
 * @file   TacticalEffect_AttackPoint.h
 * @brief  공격력(AttackPoint) 가산 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-06-26
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_AttackPoint.generated.h"

/**
 * @brief 공격력(AttackPoint)을 가산하는 이펙트
 *
 * @details
 * 대상 속성(AttackPoint)·연산(Additive)·지속(Infinite)은 이 이펙트가 정의하고,
 * 크기(magnitude)는 1로 두며 적용 측(패시브 등)이 mDynamicMagnitude로 배율을 주입한다.
 */
UCLASS()
class P_RD_API UTacticalEffect_AttackPoint : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackPoint();
};
