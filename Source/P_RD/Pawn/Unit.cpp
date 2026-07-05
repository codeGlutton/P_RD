#include "Pawn/Unit.h"
#include "Pawn/UnitModel.h"

#include "RDCollision.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"

#include "Animation/BoardActorAnimInstance.h"

AUnit::AUnit()
{
	AutoPossessAI = EAutoPossessAI::Disabled;

	mCapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	mMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	mMovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
#if WITH_EDITORONLY_DATA
	mArrowComp = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComp"));
#endif

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

	if (mMovementComp != nullptr)
	{
		mMovementComp->UpdatedComponent = mCapsuleComp;
	}

#if WITH_EDITORONLY_DATA
	if (mArrowComp != nullptr)
	{
		mArrowComp->ArrowColor = FColor(150, 200, 255);
		mArrowComp->bTreatAsASprite = true;
		mArrowComp->SetupAttachment(mCapsuleComp);
		mArrowComp->bIsScreenSizeScaled = true;
		mArrowComp->SetSimulatePhysics(false);
	}
#endif
}

void AUnit::BindModel(UObjectModel* Model)
{
	IActorView::BindModel(Model);
	mUnitModel = Cast<UUnitModel>(Model);

	// 즉시 이동 구독
	if (mUnitModel.IsValid() == true)
	{
		mUnitModel->OnPlaceTileTransform.AddWeakLambda(this, [this](const FTileTransform& TileTransform, const FTransform& Transform) {
			FVector UnitLocation = Transform.GetLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			FTransform UnitTransform = Transform;
			UnitTransform.SetLocation(UnitLocation);

			SetActorTransform(UnitTransform);
			});

		mUnitModel->OnPlayApplyAnimationUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> MotionEndBarrier, TSharedPtr<FPresentationBarrier> MotionTriggerBarrier, FGameplayTag ApplyMotionTag) {
			if (mMeshComp != nullptr)
			{
				UBoardActorAnimInstance* BoardActorAnimInst = Cast<UBoardActorAnimInstance>(mMeshComp->GetAnimInstance());
				if (BoardActorAnimInst != nullptr)
				{
					FOnTriggerEndAnimationEvent OnTriggerEndAnimationEvent;
					OnTriggerEndAnimationEvent.AddLambda([MotionEndBarrier](FGameplayTag Tag, UAnimMontage* EndAnim, bool IsInterrupted) {
						});
					BoardActorAnimInst->PlayMontageUsingTag(ApplyMotionTag, MoveTemp(OnTriggerEndAnimationEvent));

					BoardActorAnimInst->RegisterEventOnMontage();
				}
			}
			});
	}
}

void AUnit::UnbindModel(UObjectModel* Model)
{
	IActorView::UnbindModel(Model);
}

UObjectModel* AUnit::GetModel_Internal() const
{
	return mUnitModel.Get();
}

UCapsuleComponent* AUnit::GetCapsuleComponent() const
{
	return mCapsuleComp;
}

USkeletalMeshComponent* AUnit::GetMesh() const
{
	return mMeshComp;
}

UFloatingPawnMovement* AUnit::GetCharacterMovement() const
{
	return mMovementComp;
}

#if WITH_EDITORONLY_DATA
UArrowComponent* AUnit::GetArrowComponent() const
{
	return mArrowComp;
}
#endif