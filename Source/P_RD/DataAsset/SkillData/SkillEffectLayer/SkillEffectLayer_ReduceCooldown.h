/*****************************************************************//**
 * @file   SkillEffectLayer_ReduceCooldown.h
 * @brief  하나의 스킬 모션 내에서 적용하는 쿨다운 감소 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-09-21
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_ReduceCooldown.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 쿨다운 감소 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_ReduceCooldown : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;
	FText MakeDescription() const override;

public:
	UPROPERTY(Category = "Cooldown", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CooldownReduction"))
	int32 mCooldownReduction = 1;
};
