/*****************************************************************//**
 * @file   SkillEffectLayer_Pull.h
 * @brief  하나의 스킬 모션 내에서 적용하는 당기기 효과 단위 구현 헤더
 * @author 이문환
 * @date   2026-08-26
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Pull.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 당기기 효과 단위
 * @details 대상을 시전자 쪽으로 붙을 때까지 끌어옴. 거리 지정 없음
 *          (TagGain은 켜기 스위치로 1 이상이면 발동)
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Pull : public FSkillEffectLayer_TagBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UTacticalEffect> GetTagEffectClass() const override;

public:
	FText MakeDescription() const override;
};
