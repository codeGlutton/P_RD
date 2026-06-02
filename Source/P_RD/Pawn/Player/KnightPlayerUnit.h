/*****************************************************************//**
 * @file   KnightPlayerUnit.h
 * @brief  플레이어 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-11
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

#include "Pawn/Player/PlayerUnit.h"

#include "KnightPlayerUnit.generated.h"

/**
 * @brief  SRPG에서 사용되는 플레이어 폰 베이스 클래스
 */
UCLASS()
class P_RD_API AKnightPlayerUnit : public APlayerUnit
{
	GENERATED_BODY()

public:
	AKnightPlayerUnit();
};
