/*****************************************************************//**
 * @file   TacticalEffect_Acumeny.h
 * @brief  예리함 버프/디버프 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-24
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Acumeny.generated.h"

/**
 * @brief  예리함 버프 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Buff_Acumeny : public UTacticalEffect_InfiniteStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Buff_Acumeny();
};

/**
 * @brief  예리함 버프 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddBuff_Acumeny : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddBuff_Acumeny();
};

/**
 * @brief  예리함 버프 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetBuff_Acumeny : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetBuff_Acumeny();
};

/**
 * @brief  예리함 디버프 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Debuff_Acumeny : public UTacticalEffect_InfiniteStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Debuff_Acumeny();
};

/**
 * @brief  예리함 디버프 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddDebuff_Acumeny : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddDebuff_Acumeny();
};

/**
 * @brief  예리함 디버프 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetDebuff_Acumeny : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetDebuff_Acumeny();
};
