#include "Actor/BoardActor/Obstacle/CombatTargetObstacle.h"

#include "Actor/BoardActor/Obstacle/CombatTargetObstacleModel.h"
#include "Component/SkillAnimationComponent/StaticMeshSkillAnimationComponent.h"
#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"

ACombatTargetObstacle::ACombatTargetObstacle()
{
	mSkillAnimationComp = CreateDefaultSubobject<UStaticMeshSkillAnimationComponent>(TEXT("SkillAnimationComp"));
	mDissolveVFXTimelineComp = CreateDefaultSubobject<UDissolveVFXTimelineComponent>(TEXT("DissolveVFXTimelineComp"));
}

void ACombatTargetObstacle::BindModel(UObjectModel* Model)
{
	Super::BindModel(Model);
	mCombatTargetObstacleModel = Cast<UCombatTargetObstacleModel>(Model);
}

void ACombatTargetObstacle::UnbindModel(UObjectModel* Model)
{
	mCombatTargetObstacleModel.Reset();
	Super::UnbindModel(Model);
}

USkillAnimationComponent* ACombatTargetObstacle::GetSkillAnimationComponent() const
{
	return mSkillAnimationComp;
}

UDissolveVFXTimelineComponent* ACombatTargetObstacle::GetDissolveVFXTimelineComponent() const
{
	return mDissolveVFXTimelineComp;
}

UPrimitiveComponent* ACombatTargetObstacle::GetTargetMeshComponent() const
{
	return GetMesh();
}

bool ACombatTargetObstacle::IsSelectable() const
{
	return true;
}
