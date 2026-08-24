/*****************************************************************//**
 * @file   TacticalEffect_Strength.h
 * @brief  완력 버프/디버프 이펙트 정의 헤더
 * @author 모호재
 * @date   2026-08-24
 *********************************************************************/

#pragma once

#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TacticalEffect_Strength.generated.h"

/**
 * @brief  완력 버프 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Buff_Strength : public UTacticalEffect_InfiniteStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Buff_Strength();
};

/**
 * @brief  완력 버프 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddBuff_Strength : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddBuff_Strength();
};

/**
 * @brief  완력 버프 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetBuff_Strength : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetBuff_Strength();
};

/**
 * @brief  완력 디버프 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_Debuff_Strength : public UTacticalEffect_InfiniteStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_Debuff_Strength();
};

/**
 * @brief  완력 디버프 변화 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_AddDebuff_Strength : public UTacticalEffect_AddStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_AddDebuff_Strength();
};

/**
 * @brief  완력 디버프 부여 이펙트
 */
UCLASS()
class P_RD_API UTacticalEffect_GetDebuff_Strength : public UTacticalEffect_GetStatus
{
	GENERATED_BODY()

public:
	UTacticalEffect_GetDebuff_Strength();
};
