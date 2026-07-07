/*****************************************************************//**
 * @file   SkillEffectLayer_Weakness.h
 * @brief  하나의 스킬 모션 내에서 적용하는 약화 디버프 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Weakness.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 약화 디버프 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Weakness : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void CommitEffect(IBoardCombatTarget* ActorModel, const TArray<FTileIndex>& TargetTileIndexes, const TArray<IBoardCombatTarget*>& OtherCombatTargets, float DiceSum) const override;

public:
	UPROPERTY(Category = "Weakness", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultTagGain"))
	float mDefaultTagGain = 0.f;
	UPROPERTY(Category = "Weakness", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceRatio"))
	float mDiceRatio = 0.f;
};
