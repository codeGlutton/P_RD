/*****************************************************************//**
 * @file   SkillEffectLayer_Haste.h
 * @brief  하나의 스킬 모션 내에서 적용하는 신속 디버프 효과 단위 구현 헤더
 * @author 김준형
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Haste.generated.h"

 /**
  * @brief  하나의 스킬 모션 내에서 적용하는 신속 버프 효과 단위
  */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Haste : public FSkillEffectLayer_TagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

public:
	FText GetTagDisplayName() const override;
};
