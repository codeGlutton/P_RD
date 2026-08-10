#include "Actor/BoardActor/Obstacle/CombatTargetObstacle.h"

#include "RDCollision.h"

#include "Actor/BoardActor/Obstacle/CombatTargetObstacleModel.h"

#include "Components/SkeletalMeshComponent.h"
#include "Component/SkillAnimationComponent/StaticMeshSkillAnimationComponent.h"
#include "Component/SkillAnimationComponent/SkeletonSkillAnimationComponent.h"
#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"
#include "Component/BoardMovementComponent/BoardMovementPresentationComponent.h"

ACombatTargetObstacle::ACombatTargetObstacle()
{
	mCombatTargetVFXTimelineComp = CreateDefaultSubobject<UCombatTargetVFXTimelineComponent>(TEXT("CombatTargetVFXTimelineComp"));
	mMovementPresentationComp = CreateDefaultSubobject<UBoardMovementPresentationComponent>(TEXT("MovementPresentationComp"));
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
	return nullptr;
}

UCombatTargetVFXTimelineComponent* ACombatTargetObstacle::GetCombatTargetVFXTimelineComponent() const
{
	return mCombatTargetVFXTimelineComp;
}

UBoardMovementPresentationComponent* ACombatTargetObstacle::GetBoardMovementPresentationComponent() const
{
	return mMovementPresentationComp;
}

UPrimitiveComponent* ACombatTargetObstacle::GetTargetMeshComponent() const
{
	return nullptr;
}

bool ACombatTargetObstacle::IsSelectable() const
{
	return true;
}

AStaticMeshCombatTargetObstacle::AStaticMeshCombatTargetObstacle()
{
	mSkillAnimationComp = CreateDefaultSubobject<UStaticMeshSkillAnimationComponent>(TEXT("SkillAnimationComp"));
	mMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));

	if (mMeshComp != nullptr)
	{
		mMeshComp->AlwaysLoadOnClient = true;
		mMeshComp->AlwaysLoadOnServer = true;
		mMeshComp->bOwnerNoSee = false;
		mMeshComp->bCastDynamicShadow = true;
		mMeshComp->bAffectDynamicIndirectLighting = true;
		mMeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		mMeshComp->SetupAttachment(GetCapsuleComponent());
		mMeshComp->SetGenerateOverlapEvents(false);
		mMeshComp->SetCanEverAffectNavigation(false);
		mMeshComp->SetCollisionProfileName(RDCollisionProfiles::BoardActor);
		mMeshComp->SetRelativeRotation(FRotator(0., -90., 0.));
	}
}

USkillAnimationComponent* AStaticMeshCombatTargetObstacle::GetSkillAnimationComponent() const
{
	return mSkillAnimationComp;
}

UPrimitiveComponent* AStaticMeshCombatTargetObstacle::GetTargetMeshComponent() const
{
	return mMeshComp;
}

UStaticMeshComponent* AStaticMeshCombatTargetObstacle::GetMesh() const
{
	return mMeshComp;
}

ASkeletonCombatTargetObstacle::ASkeletonCombatTargetObstacle()
{
	mSkillAnimationComp = CreateDefaultSubobject<USkeletonSkillAnimationComponent>(TEXT("SkillAnimationComp"));
	mMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));

	if (mMeshComp != nullptr)
	{
		mMeshComp->AlwaysLoadOnClient = true;
		mMeshComp->AlwaysLoadOnServer = true;
		mMeshComp->bOwnerNoSee = false;
		mMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		mMeshComp->bCastDynamicShadow = true;
		mMeshComp->bAffectDynamicIndirectLighting = true;
		mMeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		mMeshComp->SetupAttachment(GetCapsuleComponent());
		mMeshComp->SetGenerateOverlapEvents(false);
		mMeshComp->SetCanEverAffectNavigation(false);
		mMeshComp->SetCollisionProfileName(RDCollisionProfiles::BoardActor);
		mMeshComp->SetRelativeRotation(FRotator(0., -90., 0.));
	}
}

USkillAnimationComponent* ASkeletonCombatTargetObstacle::GetSkillAnimationComponent() const
{
	return mSkillAnimationComp;
}

UPrimitiveComponent* ASkeletonCombatTargetObstacle::GetTargetMeshComponent() const
{
	return mMeshComp;
}

USkeletalMeshComponent* ASkeletonCombatTargetObstacle::GetMesh() const
{
	return mMeshComp;
}
