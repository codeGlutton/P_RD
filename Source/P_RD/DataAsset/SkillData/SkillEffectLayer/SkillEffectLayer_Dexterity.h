/*****************************************************************//**
 * @file   SkillEffectLayer_Dexterity.h
 * @brief  하나의 스킬 모션 내에서 적용하는 재치 버프/디버프 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-08-24
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Dexterity.generated.h"

/**
 * @brief 하나의 스킬 모션 내에서 적용하는 재치 버프/디버프 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Dexterity : public FSkillEffectLayer_AttributeTagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

public:
	FText GetTagDisplayName() const override;
};
