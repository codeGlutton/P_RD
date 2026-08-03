/*****************************************************************//**
 * @file   TacticalEffect_ActionPointFactor.h
 * @brief  ActionPointFactor 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_ActionPointFactor.generated.h"

UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_AddBase();
};

UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_AddFinal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_AddFinal();
};

UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_DivideAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_MultiplyAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_MultiplyCompound();
};

UCLASS()
class P_RD_API UTacticalEffect_ActionPointFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_ActionPointFactor_Override();
};
