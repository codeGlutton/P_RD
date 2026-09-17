#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"

namespace
{
	// @brief 우선순위 배열을 스킬 수에 맞춤 (새로 늘어난 항목은 Normal)
	void SyncSkillPriorityCount(TArray<ESkillPriority>& Priorities, int32 SkillCount)
	{
		const int32 OldCount = Priorities.Num();
		if (OldCount == SkillCount)
		{
			return;
		}

		Priorities.SetNum(SkillCount);
		for (int32 Index = OldCount; Index < SkillCount; ++Index)
		{
			Priorities[Index] = ESkillPriority::Normal;
		}
	}

	// @brief 타겟 선정 기준 오버라이드 배열을 스킬 수에 맞춤 (새로 늘어난 항목은 비워두기)
	void SyncTargetPolicyOverrideCount(TArray<FEnemyTargetPolicyOverride>& Overrides, int32 SkillCount)
	{
		const int32 OldCount = Overrides.Num();
		if (OldCount == SkillCount)
		{
			return;
		}

		Overrides.SetNum(SkillCount);
		for (int32 Index = OldCount; Index < SkillCount; ++Index)
		{
			Overrides[Index] = FEnemyTargetPolicyOverride();
		}
	}
}

void UStaticEnemyUnitSpawnData::PostLoad()
{
	Super::PostLoad();

	SyncSkillPriorityCount(mSkillPriorities, mSkillDatas.Num());
	SyncTargetPolicyOverrideCount(mTargetPolicyOverrides, mSkillDatas.Num());
}

#if WITH_EDITOR
void UStaticEnemyUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SyncSkillPriorityCount(mSkillPriorities, mSkillDatas.Num());
	SyncTargetPolicyOverrideCount(mTargetPolicyOverrides, mSkillDatas.Num());
}
#endif

