#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"

void UStaticEnemyUnitSpawnData::PostLoad()
{
	Super::PostLoad();

	if (mSkillPriorities.Num() != mSkillDatas.Num())
	{
		mSkillPriorities.SetNum(mSkillDatas.Num());
	}
}

#if WITH_EDITOR
void UStaticEnemyUnitSpawnData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (mSkillPriorities.Num() != mSkillDatas.Num())
	{
		mSkillPriorities.SetNum(mSkillDatas.Num());
	}
}
#endif

