/*****************************************************************//**
 * @file   TacticalEffect_DefenseFactor.h
 * @brief  DefenseFactor 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-02
 *********************************************************************/

#pragma once

#include "TAS/Effect/TacticalEffect.h"
#include "TacticalEffect_DefenseFactor.generated.h"

UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_AddBase : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_AddBase();
};

UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_AddFinal : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_AddFinal();
};

UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_DivideAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_DivideAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_MultiplyAdditive : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_MultiplyAdditive();
};

UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_MultiplyCompound : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_MultiplyCompound();
};

UCLASS()
class P_RD_API UTacticalEffect_DefenseFactor_Override : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTacticalEffect_DefenseFactor_Override();
};
