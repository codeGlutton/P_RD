/*****************************************************************//**
 * @file   UnitTeamType.h
 * @brief  유닛 팀 타입 대한 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "GenericTeamAgentInterface.h"
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
        Enemy               UMETA(ToolTip = "몬스터 팀"),
        NPC                 UMETA(ToolTip = "NPC 팀"),
        Count               UMETA(Hidden)
    };
}

/**
 * @brief  한 팀에서 다른 팀들을 대하는 관계 정보 구조체
 */
USTRUCT(BlueprintType)
struct FUnitTeamRelation
{
    GENERATED_BODY()

public:
    FUnitTeamRelation();
    FUnitTeamRelation(std::initializer_list<TPairInitializer<const TEnumAsByte<EUnitTeamType::Type>&, const TEnumAsByte<ETeamAttitude::Type>&>> Attitudes) : mAttitudes(MoveTemp(Attitudes)) {}

public:
    UPROPERTY(Category = Team, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Attitudes"))
    TMap<TEnumAsByte<EUnitTeamType::Type>, TEnumAsByte<ETeamAttitude::Type>> mAttitudes;
};