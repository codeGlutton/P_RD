/*****************************************************************//**
 * @file   SkillEffectLayer_GetDefense.h
 * @brief  하나의 스킬 모션 내에서 적용하는 방어도 습득 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-07-03
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_GetDefense.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 방어도 습득 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_GetDefense : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	TArray<FActiveTacticalEffectHandle> ApplyFactorEffect(IBoardCombatTarget* ActorModel) const override;
	void ClearFactorEffect(IBoardCombatTarget* ActorModel, TArray<FActiveTacticalEffectHandle>& Handles) const override;

public:
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;

#if WITH_EDITOR
public:
	FText MakeDescription() const override;
#endif

public:
	UPROPERTY(Category = "Defense", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefenseGain"))
	int32 mDefenseGain = 0;
};
