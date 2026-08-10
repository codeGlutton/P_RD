#include "Pawn/Unit.h"
#include "Pawn/UnitModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "GameMode/RDGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

#include "RDCollision.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Component/SkillAnimationComponent/SkeletonSkillAnimationComponent.h"
#include "Component/VFXTimelineComponent/VFXTimelineComponent.h"
#include "Component/BoardMovementComponent/BoardMovementPresentationComponent.h"

AUnit::AUnit()
{
	AutoPossessAI = EAutoPossessAI::Disabled;

	mCapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	mMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	mMovementPresentationComp = CreateDefaultSubobject<UBoardMovementPresentationComponent>(TEXT("MovementPresentationComp"));
	mArrowComp = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComp"));
	mSkillAnimationComp = CreateDefaultSubobject<USkeletonSkillAnimationComponent>(TEXT("SkillAnimationComp"));
	mCombatTargetVFXTimelineComp = CreateDefaultSubobject<UCombatTargetVFXTimelineComponent>(TEXT("CombatTargetVFXTimelineComp"));

	if (mCapsuleComp != nullptr)
	{
		mCapsuleComp->InitCapsuleSize(34.0f, 88.0f);
		mCapsuleComp->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

		mCapsuleComp->CanCharacterStepUpOn = ECB_No;
		mCapsuleComp->SetShouldUpdatePhysicsVolume(true);
		mCapsuleComp->SetCanEverAffectNavigation(false);
		mCapsuleComp->bDynamicObstacle = true;
		RootComponent = mCapsuleComp;
	}

	if (mMeshComp != nullptr)
	{
		mMeshComp->AlwaysLoadOnClient = true;
		mMeshComp->AlwaysLoadOnServer = true;
		mMeshComp->bOwnerNoSee = false;
		mMeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		mMeshComp->bCastDynamicShadow = true;
		mMeshComp->bAffectDynamicIndirectLighting = true;
		mMeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		mMeshComp->SetupAttachment(mCapsuleComp);
		mMeshComp->SetGenerateOverlapEvents(false);
		mMeshComp->SetCanEverAffectNavigation(false);
		mMeshComp->SetCollisionProfileName(RDCollisionProfiles::BoardActor);
		mMeshComp->SetRelativeRotation(FRotator(0., -90., 0.));
	}

	if (mArrowComp != nullptr)
	{
		mArrowComp->ArrowColor = FColor(150, 200, 255);
		mArrowComp->bTreatAsASprite = true;
		mArrowComp->SetupAttachment(mCapsuleComp);
		mArrowComp->bIsScreenSizeScaled = true;
		mArrowComp->SetSimulatePhysics(false);
	}
}

void AUnit::BeginPlay()
{
	Super::BeginPlay();

	// 유닛은 캡슐 반높이만큼 띄워서 바닥을 딛도록 함 (BP에서 캡슐 크기를 바꿀 수 있으므로 런타임에 전달)
	if (mMovementPresentationComp != nullptr && mCapsuleComp != nullptr)
	{
		mMovementPresentationComp->SetGroundZOffset(mCapsuleComp->GetScaledCapsuleHalfHeight());
	}
}

void AUnit::BindModel(UObjectModel* Model)
{
	IActorView::BindModel(Model);
	mUnitModel = Cast<UUnitModel>(Model);

	// 연출 요청 구독
	if (mUnitModel.IsValid())
	{
		// 위치 가져오기 구독
		mUnitModel->OnGetBoardActorWorldTransform.BindUObject(this, &AUnit::GetActorTransform);
	}
}

void AUnit::UnbindModel(UObjectModel* Model)
{
	// @note 이동 연출 컴포넌트의 구독 해제와 상태 정리는 IActorView::UnbindModel이 자동 전파
	IActorView::UnbindModel(Model);

	if (mUnitModel.IsValid() == true)
	{
		mUnitModel->OnGetBoardActorWorldTransform.Unbind();
	}

	mUnitModel.Reset();
}

UObjectModel* AUnit::GetModel_Internal() const
{
	return mUnitModel.Get();
}

USkillAnimationComponent* AUnit::GetSkillAnimationComponent() const
{
	return mSkillAnimationComp;
}

UCombatTargetVFXTimelineComponent* AUnit::GetCombatTargetVFXTimelineComponent() const
{
	return mCombatTargetVFXTimelineComp;
}

UPrimitiveComponent* AUnit::GetTargetMeshComponent() const
{
	return mMeshComp;
}

FVector AUnit::GetVelocity() const
{
	// 이동 연출 컴포넌트가 계산해서 저장한 현재 속도를 반환
	return (mMovementPresentationComp != nullptr)
		? mMovementPresentationComp->GetMoveVelocity()
		: FVector::ZeroVector;
}

UCapsuleComponent* AUnit::GetCapsuleComponent() const
{
	return mCapsuleComp;
}

USkeletalMeshComponent* AUnit::GetMesh() const
{
	return mMeshComp;
}

UArrowComponent* AUnit::GetArrowComponent() const
{
	return mArrowComp;
}

UBoardMovementPresentationComponent* AUnit::GetBoardMovementPresentationComponent() const
{
	return mMovementPresentationComp;
}
