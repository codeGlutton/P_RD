/*****************************************************************//**
 * @file   TacticalEffect_SpeedPointFactor.h
 * @brief  SpeedPointFactor 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_SpeedPointFactor.generated.h"

UCLASS()
class P_RD_API UTacticalEffect_SpeedPointFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPointFactor_AddBase();
};

UCLASS()
class P_RD_API UTacticalEffect_SpeedPointFactor_AddFinal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPointFactor_AddFinal();
};

UCLASS()
class P_RD_API UTacticalEffect_SpeedPointFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPointFactor_DivideAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_SpeedPointFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPointFactor_MultiplyAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_SpeedPointFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPointFactor_MultiplyCompound();
};

UCLASS()
class P_RD_API UTacticalEffect_SpeedPointFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_SpeedPointFactor_Override();
};
