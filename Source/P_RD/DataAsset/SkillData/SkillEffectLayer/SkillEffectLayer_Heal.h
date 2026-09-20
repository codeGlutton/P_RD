/*****************************************************************//**
 * @file   SkillEffectLayer_Heal.h
 * @brief  하나의 스킬 모션 내에서 적용하는 힐 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-07-06
 *********************************************************************/

#pragma once

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "SkillEffectLayer_Heal.generated.h"

/**
 * @brief  하나의 스킬 모션 내에서 적용하는 힐 효과 단위
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_Heal : public FSkillEffectLayer
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
	UPROPERTY(Category = "Heal", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "HealGain"))
	int32 mHealGain = 0;
};
