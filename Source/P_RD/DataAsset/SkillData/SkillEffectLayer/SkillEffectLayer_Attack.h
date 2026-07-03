/*****************************************************************//**
 * @file   SkillEffectLayer_Attack.h
 * @brief  하나의 스킬 모션 내에서 적용하는 단일 공격 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-07-03
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Attack.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 단일 공격 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Attack : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void ClearPointEffect(IBoardCombatTarget* ActorModel) const override;

public:
	void ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const override;
	void CommitEffect(IBoardCombatTarget* ActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const override;
};
