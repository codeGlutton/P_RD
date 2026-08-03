/*****************************************************************//**
 * @file   TacticalEffect_CriticalFactor.h
 * @brief  CriticalFactor 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_CriticalFactor.generated.h"

UCLASS()
class P_RD_API UTacticalEffect_CriticalFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_CriticalFactor_AddBase();
};

UCLASS()
class P_RD_API UTacticalEffect_CriticalFactor_AddFinal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_CriticalFactor_AddFinal();
};

UCLASS()
class P_RD_API UTacticalEffect_CriticalFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_CriticalFactor_DivideAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_CriticalFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_CriticalFactor_MultiplyAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_CriticalFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_CriticalFactor_MultiplyCompound();
};

UCLASS()
class P_RD_API UTacticalEffect_CriticalFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_CriticalFactor_Override();
};
