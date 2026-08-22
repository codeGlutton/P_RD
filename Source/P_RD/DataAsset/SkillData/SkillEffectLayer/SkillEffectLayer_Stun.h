/*****************************************************************//**
 * @file   SkillEffectLayer_Stun.h
 * @brief  하나의 스킬 모션 내에서 적용하는 기절 디버프 효과 단위 구현 헤더
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Stun.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 기절 디버프 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Stun : public FSkillEffectLayer_TagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

public:
	FText GetTagDisplayName() const override;
};
