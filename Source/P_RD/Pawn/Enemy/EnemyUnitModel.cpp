/*****************************************************************//**
 * @file   EnemyUnitModel.cpp
 * @brief  적 베이스 유닛 모델 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Setting/GameTeamType.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UEnemyUnitModel::UEnemyUnitModel()
{
	UUnitModel::SetGenericTeamId(EGameTeamType::Enemy);

	// 적 스탯 세트 생성 — 속성 컴포넌트가 자식 AttributeSet을 자동 수집해 스탯 커브 초기화 대상이 된다(플레이어와 동일 패턴).
	mUnitAttributeSet = CreateDefaultSubobject<UEnemyUnitAttributeSet>(TEXT("EnemyUnitAttributeSet"));
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
		SkillComp->SetSkillFrom(EnemySpawn->mSkillDatas);
	}
}

int32 UEnemyUnitModel::GetBoardActorLevel() const
{
	return GetDifficulty();
}

void UEnemyUnitModel::SetDifficulty(int32 Difficulty)
{
	mDifficulty = Difficulty;
}

int32 UEnemyUnitModel::GetDifficulty() const
{
	return mDifficulty;
}

EMoveTendency UEnemyUnitModel::GetMoveTendency() const
{
	return mMoveTendency;
}

