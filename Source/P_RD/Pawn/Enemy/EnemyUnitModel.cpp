#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Setting/GameTeamType.h"
#include "AttributeSet/UnitAttributeSet.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"

#include "Component/SkillComponent/SkillComponentModel.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h"
#include "DataAsset/SkillData/StaticUnitSkillData.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Component/EquipmentComponent/EquipmentComponentModel.h"

UEnemyUnitModel::UEnemyUnitModel()
{
	UUnitModel::SetGenericTeamId(EGameTeamType::Enemy);

	mEquipmentCompModel = CreateDefaultSubobject<UEquipmentComponentModel>(TEXT("EquipmentComponentModel"));
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
	if (UEquipmentComponentModel* EquipComp = GetEquipmentComponentModel())
	{
		EquipComp->EquipFrom(EnemySpawn->mEquipmentDatas);
	}
}

int32 UEnemyUnitModel::GetBoardActorLevel() const
{
	return GetDifficulty();
}

UEquipmentComponentModel* UEnemyUnitModel::GetEquipmentComponentModel() const
{
	return mEquipmentCompModel;
}

void UEnemyUnitModel::SetDifficulty(int32 Difficulty)
{
	mDifficulty = Difficulty;

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

	TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), GetBoardActorKeyName(), GetDifficulty(), true);
}

int32 UEnemyUnitModel::GetDifficulty() const
{
	return mDifficulty;
}

EMoveTendency UEnemyUnitModel::GetMoveTendency() const
{
	return mMoveTendency;
}

