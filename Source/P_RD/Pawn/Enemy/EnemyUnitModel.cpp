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

UEnemyUnitModel::UEnemyUnitModel()
{
	UUnitModel::SetGenericTeamId(EGameTeamType::Enemy);

	// 적 스탯 세트 생성 — 속성 컴포넌트가 자식 AttributeSet을 자동 수집해 스탯 커브 초기화 대상이 된다(플레이어와 동일 패턴).
	mUnitAttributeSet = CreateDefaultSubobject<UUnitAttributeSet>(TEXT("UnitAttributeSet"));
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
	mMovePoint = EnemySpawn->mMovePoint;

	if (USkillComponentModel* SkillComp = GetSkillComponentModel())
	{
		// 스폰 데이터의 스킬을 로드해서 슬롯 0부터 순서대로 장착
		int32 SkillIndex = 0;
		for (const TSoftObjectPtr<UStaticSkillData>& SkillSoft : EnemySpawn->mSkillDatas)
		{
			// 로드 성공한 스킬만 슬롯에 세팅
			if (UStaticSkillData* Loaded = SkillSoft.LoadSynchronous())
			{
				SkillComp->SetSkill(SkillIndex++, Loaded);
			}
		}
	}
}

int32 UEnemyUnitModel::GetDifficulty() const
{
	return mDifficulty;
}

EMoveTendency UEnemyUnitModel::GetMoveTendency() const
{
	return mMoveTendency;
}

int32 UEnemyUnitModel::GetMovePoint() const
{
	return mMovePoint;
}
