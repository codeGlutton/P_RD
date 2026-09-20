/*****************************************************************//**
 * @file   TacticalEffect_HealFactor.h
 * @brief  HealFactor 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_HealFactor.generated.h"

UCLASS()
class P_RD_API UTacticalEffect_HealFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_AddBase();
};

UCLASS()
class P_RD_API UTacticalEffect_HealFactor_AddFinal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_AddFinal();
};

UCLASS()
class P_RD_API UTacticalEffect_HealFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_DivideAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_HealFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_MultiplyAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_HealFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_MultiplyCompound();
};

UCLASS()
class P_RD_API UTacticalEffect_HealFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_HealFactor_Override();
};
