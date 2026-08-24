/*****************************************************************//**
 * @file   TacticalEffect_Dexterity.h
 * @brief  재치 버프/디버프 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-24
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Dexterity.generated.h"

/**
 * @brief  재치 버프 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Buff_Dexterity : public UTacticalEffect_InfiniteStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Buff_Dexterity();
};

/**
 * @brief  재치 버프 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddBuff_Dexterity : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddBuff_Dexterity();
};

/**
 * @brief  재치 버프 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetBuff_Dexterity : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetBuff_Dexterity();
};

/**
 * @brief  재치 디버프 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Debuff_Dexterity : public UTacticalEffect_InfiniteStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Debuff_Dexterity();
};

/**
 * @brief  재치 디버프 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddDebuff_Dexterity : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddDebuff_Dexterity();
};

/**
 * @brief  재치 디버프 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetDebuff_Dexterity : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetDebuff_Dexterity();
};
