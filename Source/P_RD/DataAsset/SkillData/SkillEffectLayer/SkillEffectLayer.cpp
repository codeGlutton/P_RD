#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

FSkillEffectCommitParams::FSkillEffectCommitParams(
	TScriptInterface<IBoardCombatTarget> Instigator,
	TObjectPtr<UBoardCombatTargetSnapshotData> InstigatorSnapshot,
	TArray<TScriptInterface<IBoardCombatTarget>>& Targets,
	TArray<TObjectPtr<UBoardCombatTargetSnapshotData>>& TargetSnapshots,
	TArray<FTileIndex>& TargetTileIndexes,
	float DiceSum
) :
	mInstigator(Instigator),
	mInstigatorSnapshot(InstigatorSnapshot),
	mTargets(Targets),
	mTargetSnapshots(TargetSnapshots),
	mTargetTileIndexes(TargetTileIndexes),
	mDiceSum(DiceSum)
{

}

