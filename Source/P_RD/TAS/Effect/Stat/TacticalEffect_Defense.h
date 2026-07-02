/*****************************************************************//**
 * @file   TacticalEffect_Defense.h
 * @brief  Defense 이펙트 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_Defense.generated.h"

/**
 * @brief Defense 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Defense : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_Defense();
};
