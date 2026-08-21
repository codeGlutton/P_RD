/*****************************************************************//**
 * @file   SkillEffectLayer_Push.h
 * @brief  하나의 스킬 모션 내에서 적용하는 밀치기 효과 단위 구현 헤더
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Push.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 밀치기 효과 단위
 * @details 시전자(스킬을 발동한 유닛 또는 기믹)가 바라보는 방향으로 대상을 밀어냄.
 *          발판형 기믹은 배치 방향이 곧 미는 방향.
 *          이동만 담당하고 데미지는 Attack 레이어를 함께 사용
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Push : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;

#if WITH_EDITOR
public:
	FText MakeDescription() const override;
#endif

public:
	// @brief 최대 밀치기 칸 수 (뒤가 막히면 막히기 직전까지 밀림)
	UPROPERTY(Category = "Push", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PushDistance", ClampMin = "1"))
	int32 mPushDistance = 1;
};
