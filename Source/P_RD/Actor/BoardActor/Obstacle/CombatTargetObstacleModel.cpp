#include "Actor/BoardActor/Obstacle/CombatTargetObstacleModel.h"
#include "Setting/GameTeamType.h"

#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"

#include "Singleton/WorldSubsystem/TacticalFrameworkModel.h"
#include "AttributeSet/CombatTargetAttributeSet.h"

#include "DataAsset/ObstacleSpawnData/StaticCombatTargetObstacleSpawnData.h"

UCombatTargetObstacleModel::UCombatTargetObstacleModel() : mTeamId(EGameTeamType::AllNeutral)
{
	mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
	mSkillCompModel = CreateDefaultSubobject<USkillComponentModel>(TEXT("SkillComponentModel"));
	mMovementCompModel = CreateDefaultSubobject<UBoardMovementComponentModel>(TEXT("BoardMovementComponentModel"));
	mCombatTargetAttributeSet = CreateDefaultSubobject<UCombatTargetAttributeSet>(TEXT("CombatTargetAttributeSet"));

	mTileLayerFlags = StaticCast<int32>(ETileLayerFlag::Obstacle);
	mBlockLayerFlags = StaticCast<int32>(ETileLayerFlag::Unit | ETileLayerFlag::Obstacle);
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
	mOverlayLayerPriority = 0;
}

void UCombatTargetObstacleModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	// 스폰 데이터 획득
	const UStaticCombatTargetObstacleSpawnData* ObstacleSpawn = Cast<UStaticCombatTargetObstacleSpawnData>(mStaticSpawnData);
	if (ObstacleSpawn == nullptr)
	{
		return;
	}

	if (USkillComponentModel* SkillComp = GetSkillComponentModel())
	{
		SkillComp->SetSkillFrom(ObstacleSpawn->mSkillDatas);
	}
}

void UCombatTargetObstacleModel::OnEndRoom()
{
	Super::OnEndRoom();

	/* 모두 제거 */

	mAttributeCompModel->RemoveActiveEffectsWithTags(FGameplayTagContainer(EffectTags::GameplayEffect_StatusEffect));
	mAttributeCompModel->ApplyModToAttribute(UCombatTargetAttributeSet::GetDefenseAttribute(), ETacticalModOp::Override, 0.f);
}

UAttributeSetComponentModel* UCombatTargetObstacleModel::GetAttributeComponentModel() const
{
	return mAttributeCompModel;
}

USkillComponentModel* UCombatTargetObstacleModel::GetSkillComponentModel() const
{
	return mSkillCompModel;
}

UBoardMovementComponentModel* UCombatTargetObstacleModel::GetBoardMovementComponentModel() const
{
	return mMovementCompModel;
}

void UCombatTargetObstacleModel::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	mTeamId = TeamID;
}

FGenericTeamId UCombatTargetObstacleModel::GetGenericTeamId() const
{
	return mTeamId;
}

void UCombatTargetObstacleModel::SetDifficulty(int32 Difficulty)
{
	mDifficulty = Difficulty;

	UTacticalFrameworkModel* TacticalFrameworkModel = GetWorldSubsystemModel<UTacticalFrameworkModel>(this);
	checkf(TacticalFrameworkModel != nullptr, TEXT("전략 프레임워크 모델 nullptr"));

	TacticalFrameworkModel->GetAttributeSetInitter()->InitAttributeSetDefaults(GetAttributeComponentModel(), GetBoardActorKeyName(), GetDifficulty(), true);
}

int32 UCombatTargetObstacleModel::GetDifficulty() const
{
	return mDifficulty;
}

