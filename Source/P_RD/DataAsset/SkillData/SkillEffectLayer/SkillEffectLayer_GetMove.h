/*****************************************************************//**
 * @file   SkillEffectLayer_GetMove.h
 * @brief  하나의 스킬 모션 내에서 적용하는 기동력 습득 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-07-03
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_GetMove.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 기동력 습득 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_GetMove : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void ApplyPointEffect(IBoardCombatTarget* ActorModel, float DiceSum) const override;
	void ClearPointEffect(IBoardCombatTarget* ActorModel) const override;

public:
	FActiveTacticalEffectHandle ApplyFactorEffect(IBoardCombatTarget* ActorModel) const override;
	void ClearFactorEffect(IBoardCombatTarget* ActorModel, FActiveTacticalEffectHandle Handle) const override;

public:
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;

public:
	UPROPERTY(Category = "Move", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultMoveGain"))
	float mDefaultMoveGain = 0.f;
	UPROPERTY(Category = "Move", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceRatio"))
	float mDiceRatio = 0.f;
};
