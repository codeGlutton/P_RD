/*****************************************************************//**
 * @file   SkillEffectLayer.h
 * @brief  하나의 스킬 모션 내에서 적용하는 단일 효과 단위 구현 헤더
 * @author 모호재
 * @date   2026-06-30
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "SkillEffectLayer.generated.h"

class IBoardCombatTarget;
class UBoardCombatTargetSnapshotData;

struct FSkillEffectCommitParams
{
public:
	FSkillEffectCommitParams(
		TScriptInterface<IBoardCombatTarget> Instigator,
		TObjectPtr<UBoardCombatTargetSnapshotData> InstigatorSnapshot,
		TArray<TScriptInterface<IBoardCombatTarget>>& Targets,
		TArray<TObjectPtr<UBoardCombatTargetSnapshotData>>& TargetSnapshots,
		TArray<FTileIndex>& TargetTileIndexes
	);

public:
	TScriptInterface<IBoardCombatTarget> mInstigator = nullptr;
	TObjectPtr<UBoardCombatTargetSnapshotData> mInstigatorSnapshot = nullptr;

	TArray<TScriptInterface<IBoardCombatTarget>>& mTargets;
	TArray<TObjectPtr<UBoardCombatTargetSnapshotData>>& mTargetSnapshots;

public:
	const TArray<FTileIndex>& mTargetTileIndexes;
};

USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer
{
	GENERATED_BODY()

public:
	virtual ~FSkillEffectLayer() = default;

public:
	virtual void ApplyPointEffect(IBoardCombatTarget* ActorModel) const {}
	virtual void ClearPointEffect(IBoardCombatTarget* ActorModel) const {}

public:
	virtual FActiveTacticalEffectHandle ApplyFactorEffect(IBoardCombatTarget* ActorModel) const 
	{ 
		return FActiveTacticalEffectHandle();
	}
	virtual void ClearFactorEffect(IBoardCombatTarget* ActorModel, FActiveTacticalEffectHandle Handle) const {}

public:
	virtual void CommitEffect(const FSkillEffectCommitParams& Params) const PURE_VIRTUAL(FSkillEffectLayer::CommitEffect, return; );
};

USTRUCT(BlueprintType)
struct P_RD_API FSkillEffectLayer_TagBase : public FSkillEffectLayer
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<UTacticalEffect> GetTagEffectClass() const;
	void CommitEffect(const FSkillEffectCommitParams& Params) const override;

public:
	UPROPERTY(Category = "Tag", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TagGain"))
	int32 mTagGain = 0;
};