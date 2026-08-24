/*****************************************************************//**
 * @file   SkillEffectLayer_GetActionPoint.h
 * @brief  하나의 스킬 모션 내에서 적용하는 행동력 습득 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_GetActionPoint.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 행동력 습득 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_GetActionPoint : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	TArray<FActiveTacticalEffectHandle> ApplyFactorEffect(IBoardCombatTarget* ActorModel, const UBoardCombatTargetSnapshotData* Snapshot) const override;
	void ClearFactorEffect(IBoardCombatTarget* ActorModel, TArray<FActiveTacticalEffectHandle>& Handles) const override;

public:
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;

public:
	FText MakeDescription() const override;

public:
	UPROPERTY(Category = "ActionPoint", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ActionPointGain"))
	int32 mActionPointGain = 0;
};
