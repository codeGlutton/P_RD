/*****************************************************************//**
 * @file   KnightPlayerUnitModel.h
 * @brief  기사 플레이어 유닛 정의 헤더
 * @author 모호재
 * @date   2026-05-11
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "KnightPlayerUnitModel.generated.h"

/**
 * @brief  SRPG에서 사용되는 플레이어 폰 베이스 클래스
 */
UCLASS()
class P_RD_API UKnightPlayerUnitModel : public UPlayerUnitModel
{
	GENERATED_BODY()

public:
	UKnightPlayerUnitModel();
};
