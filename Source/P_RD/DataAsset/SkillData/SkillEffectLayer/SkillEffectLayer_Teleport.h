/*****************************************************************//**
 * @file   SkillEffectLayer_Teleport.h
 * @brief  하나의 스킬 모션 내에서 적용하는 순간이동 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-09-11
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Teleport.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 순간이동 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Teleport : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;
	FText MakeDescription() const override;
};

