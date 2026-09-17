#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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
}

void UStaticEnemyUnitSpawnData::PostLoad()
{
	Super::PostLoad();

	SyncSkillPriorityCount(mSkillPriorities, mSkillDatas.Num());
}

#if WITH_EDITOR
EDataValidationResult UStaticEnemyUnitSpawnData::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);
	if (mTargetPolicy.IsStatusPolicy() && !mTargetPolicy.mStatusTag.IsValid())
	{
		Context.AddError(FText::FromString(TEXT("AI 상태이상 대상 우선순위에 Status Tag를 지정하세요.")));
		return EDataValidationResult::Invalid;
	}
	return Result;
}

void UStaticEnemyUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SyncSkillPriorityCount(mSkillPriorities, mSkillDatas.Num());
}
#endif

