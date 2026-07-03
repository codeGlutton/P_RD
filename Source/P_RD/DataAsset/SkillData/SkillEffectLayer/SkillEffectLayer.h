/*****************************************************************//**
 * @file   SkillEffectLayer.h
 * @brief  하나의 스킬 모션 내에서 적용하는 단일 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-06-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SkillEffectLayer.generated.h"

class IBoardCombatTarget;

USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer
{
	GENERATED_BODY()

public:
	virtual ~FSkillEffectLayer() = default;

public:
	virtual void ClearPointEffect(IBoardCombatTarget* ActorModel) const PURE_VIRTUAL(FSkillEffectLayer::ClearPointEffect, return; );

public:
	virtual void ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const PURE_VIRTUAL(FSkillEffectLayer::ApplyPointEffect, return; );
	virtual void CommitEffect(IBoardCombatTarget* ActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const PURE_VIRTUAL(FSkillEffectLayer::CommitEffect, return; );
};
