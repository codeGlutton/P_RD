/*****************************************************************//**
 * @file   TileTargetable.h
 * @brief  SRPG 타일에 타격 가능한 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "UObject/Interface.h"
#include "TileTargetable.generated.h"

UINTERFACE(MinimalAPI)
class UTileTargetable : public UAbilitySystemInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 타일에 타격 가능한 객체
 */
class P_RD_API ITileTargetable : public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	virtual bool IsTargetable() const;
};
