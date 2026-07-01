/*****************************************************************//**
 * @file   EnemyUnitModel.cpp
 * @brief  적 베이스 유닛 모델 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Setting/GameTeamType.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/SkillData/StaticSkillData.h"

UEnemyUnitModel::UEnemyUnitModel()
{
	UUnitModel::SetGenericTeamId(EGameTeamType::Enemy);
}

void UEnemyUnitModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	// 스폰 데이터 획득
	const UStaticEnemyUnitSpawnData* EnemySpawn = Cast<UStaticEnemyUnitSpawnData>(mStaticSpawnData);
	if (EnemySpawn == nullptr)
	{
		return;
	}

	mMoveTendency = EnemySpawn->mMoveTendency;

	if (USkillComponentModel* SkillComp = GetSkillComponentModel())
	{
		for (const TSoftObjectPtr<UStaticSkillData>& SkillSoft : EnemySpawn->mSkillDatas)
		{
			// 스킬 로드되면 스킬컴포넌트에 적재
			if (SkillSoft.LoadSynchronous() != nullptr)
				SkillComp->AddSkillData(SkillSoft);
		}
	}
}

int32 UEnemyUnitModel::GetDifficulty() const
{
	return mDifficulty;
}

bool UEnemyUnitModel::IsPlayerUnitModel() const
{
	return false;
}

EMoveTendency UEnemyUnitModel::GetMoveTendency() const
{
	return mMoveTendency;
}

UUserWidget* UEnemyUnitModel::GetInfoPanel() const
{
	return nullptr;
}
