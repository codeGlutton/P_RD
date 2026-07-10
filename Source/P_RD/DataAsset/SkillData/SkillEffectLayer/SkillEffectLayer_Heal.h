/*****************************************************************//**
 * @file   SkillEffectLayer_Heal.h
 * @brief  하나의 스킬 모션 내에서 적용하는 힐 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Heal.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 힐 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Heal : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void ClearPointEffect(IBoardCombatTarget* ActorModel) const override;

public:
	void ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const override;
	void CommitEffect(IBoardCombatTarget* ActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const override;

public:
	UPROPERTY(Category = "Heal", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultHealGain"))
	float mDefaultHealGain = 0.f;
	UPROPERTY(Category = "Heal", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceRatio"))
	float mDiceRatio = 0.f;
};
