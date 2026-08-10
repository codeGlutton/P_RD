/*****************************************************************//**
 * @file   SkillEffectLayer_Frail.h
 * @brief  하나의 스킬 모션 내에서 적용하는 손상 디버프 효과 단위 구현 헤더
 * @author 김준형
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Frail.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 손상 디버프 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Frail : public FSkillEffectLayer_TagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

#if WITH_EDITOR
public:
	FText GetTagDisplayName() const override;
#endif
};