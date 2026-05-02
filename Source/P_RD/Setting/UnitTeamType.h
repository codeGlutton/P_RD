/*****************************************************************//**
 * @file   UnitTeamType.h
 * @brief  유닛 팀 타입 대한 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UnitTeamType.generated.h"

/**
 * @brief  SRPG Unit에 대한 팀 타입
 */
UENUM(BlueprintType)
namespace EUnitTeamType
{
    enum Type : uint8
    {
        AllNeutral = 0      UMETA(ToolTip = "중립 팀"),
        AllHostile          UMETA(ToolTip = "자신 포함 모두 적대하는 팀"),
        Adventurer          UMETA(ToolTip = "플레이어 팀"),
        Monster             UMETA(ToolTip = "몬스터 팀"),
        NPC                 UMETA(ToolTip = "NPC 팀"),
        Count               UMETA(Hidden)
    };
}