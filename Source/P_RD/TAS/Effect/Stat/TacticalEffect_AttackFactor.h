/*****************************************************************//**
 * @file   TacticalEffect_AttackFactor.h
 * @brief  AttackFactor 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_AttackFactor.generated.h"

UCLASS()
class P_RD_API UTacticalEffect_AttackFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor_AddBase();
};

UCLASS()
class P_RD_API UTacticalEffect_AttackFactor_AddFinal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor_AddFinal();
};

UCLASS()
class P_RD_API UTacticalEffect_AttackFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor_DivideAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_AttackFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor_MultiplyAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_AttackFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor_MultiplyCompound();
};

UCLASS()
class P_RD_API UTacticalEffect_AttackFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_AttackFactor_Override();
};
