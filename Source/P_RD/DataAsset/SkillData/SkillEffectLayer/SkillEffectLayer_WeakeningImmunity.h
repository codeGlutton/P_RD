/*****************************************************************//**
 * @file   SkillEffectLayer_WeakeningImmunity.h
 * @brief  하나의 스킬 모션 내에서 적용하는 쇠약 면역 버프 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-09-15
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_WeakeningImmunity.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 쇠약 면역 버프 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_WeakeningImmunity : public FSkillEffectLayer_TagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

public:
	FText GetTagDisplayName() const override;
};
