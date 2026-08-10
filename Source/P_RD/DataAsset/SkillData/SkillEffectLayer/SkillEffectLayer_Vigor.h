/*****************************************************************//**
 * @file   SkillEffectLayer_Vigor.h
 * @brief  하나의 스킬 모션 내에서 적용하는 활력 버프 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Vigor.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 신속 버프 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Vigor : public FSkillEffectLayer_TagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

#if WITH_EDITOR
public:
	FText GetTagDisplayName() const override;
#endif
};
