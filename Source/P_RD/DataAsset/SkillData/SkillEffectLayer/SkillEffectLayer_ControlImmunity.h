/*****************************************************************//**
 * @file   SkillEffectLayer_ControlImmunity.h
 * @brief  하나의 스킬 모션 내에서 적용하는 억제 면역 버프 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-09-15
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_ControlImmunity.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 억제 면역 버프 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_ControlImmunity : public FSkillEffectLayer_UniqueTagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

public:
	FText GetTagDisplayName() const override;
};
