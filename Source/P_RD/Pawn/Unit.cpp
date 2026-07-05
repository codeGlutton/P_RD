#include "Pawn/Unit.h"
#include "Pawn/UnitModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "RDCollision.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"

#include "Animation/BoardActorAnimInstance.h"

AUnit::AUnit()
{
	AutoPossessAI = EAutoPossessAI::Disabled;

	// @brief 이동 연출용 틱
	// @note 평소엔 꺼두고 이동스텝 수신 시에만 켜고 가만히 있을때는 끔
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

UObjectModel* AUnit::GetModel_Internal() const
{
	return mUnitModel.Get();
}

void AUnit::BindModel(UObjectModel* Model)
{
	IActorView::BindModel(Model);
	mUnitModel = Cast<UUnitModel>(Model);

	// 연출 요청 구독
	if (mUnitModel.IsValid())
	{
		// 초기 배치 연출 요청 구독
		mUnitModel->OnPlaceTileTransform.AddLambda([this](const FTileTransform& TileTransform, const FTransform& Transform) {
			FVector UnitLocation = Transform.GetLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			FTransform UnitTransform = Transform;
			UnitTransform.SetLocation(UnitLocation);

			SetActorTransform(UnitTransform);
			});

		// 이동 연출 요청 구독
		mUnitModel->OnStartMoveStep.AddUObject(this, &AUnit::OnStartMoveStep);
		
		// 스킬 Effect 타격 연출 구독
		mUnitModel->OnPlayApplyAnimationUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> MotionEndBarrier, TSharedPtr<FPresentationBarrier> MotionTriggerBarrier, FGameplayTag ApplyMotionTag, ETileActorDirection LocalDirection) {
			if (mMeshComp != nullptr)
			{
				UBoardActorAnimInstance* BoardActorAnimInst = Cast<UBoardActorAnimInstance>(mMeshComp->GetAnimInstance());
				if (BoardActorAnimInst != nullptr)
				{
					/* 스킬 연출 유지를 위한 Barrier */

					FOnTriggerEndAnimationEvent OnTriggerEndAnimationEvent;
					OnTriggerEndAnimationEvent.AddLambda([MotionEndBarrier](FGameplayTag Tag, ETileActorDirection LocalDir, UAnimMontage* EndAnim, bool IsInterrupted) {
						});
					BoardActorAnimInst->PlayMontageUsingTag(ApplyMotionTag, LocalDirection, MoveTemp(OnTriggerEndAnimationEvent));

					/* 피격 적용 대기를 위한 Barrier */

					FBoardActorAnimationEvent HitEvent;
					HitEvent.OnTriggerAnimationEvent.AddLambda([MotionTriggerBarrier](FGameplayTag Tag, ETileActorDirection LocalDir, UAnimMontage* EndAnim) {
						});
					HitEvent.mIsOneTimeEvent = true;
					BoardActorAnimInst->RegisterTagEventOnMontage(AnimationTags::Animation_Event_Hit, MoveTemp(HitEvent));
				}
			}
			});

		// 스킬 Effect 피격 연출 구독
		mUnitModel->OnPlayReceiveAnimationUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> MotionEndBarrier, FGameplayTag ReceiveMotionTag, ETileActorDirection LocalDirection) {
			if (mMeshComp != nullptr)
			{
				UBoardActorAnimInstance* BoardActorAnimInst = Cast<UBoardActorAnimInstance>(mMeshComp->GetAnimInstance());
				if (BoardActorAnimInst != nullptr)
				{
					/* 스킬 연출 유지를 위한 Barrier */

					FOnTriggerEndAnimationEvent OnTriggerEndAnimationEvent;
					OnTriggerEndAnimationEvent.AddLambda([MotionEndBarrier](FGameplayTag Tag, ETileActorDirection LocalDir, UAnimMontage* EndAnim, bool IsInterrupted) {
						});
					BoardActorAnimInst->PlayMontageUsingTag(ReceiveMotionTag, LocalDirection, MoveTemp(OnTriggerEndAnimationEvent));
				}
			}
			});
	}
}

void AUnit::UnbindModel(UObjectModel* Model)
{
	IActorView::UnbindModel(Model);

	if (mUnitModel.IsValid())
	{
		mUnitModel->OnStartMoveStep.RemoveAll(this);
	}
	mUnitModel.Reset();

	// 진행 중이던 이동스텝 연출이 있으면 배리어를 놓아서 완료로 처리
	mMoveBarrier.Reset();
	// 이동이 멈추면 틱도 비활성화
	SetActorTickEnabled(false);
}

void AUnit::OnStartMoveStep(const FTileTransform& NextTileTransform, const FTransform& TargetWorldTransform, TSharedPtr<FPresentationBarrier> Barrier)
{
	// 목표타일의 월드트랜스폼과 배리어 보관
	mMoveTargetTransform = TargetWorldTransform;
	mMoveBarrier = Barrier;

	// 타일 월드트랜스폼의 Z축은 타일 바닥이 기준이므로 액터의 중심이 타일 바닥에 파묻힘
	// 액터의 캡슐 중심만큼 더해서 바닥을 딛고 있도록 변경
	if (mCapsuleComp != nullptr)
	{
		FVector TargetLocation = mMoveTargetTransform.GetLocation();
		TargetLocation.Z += mCapsuleComp->GetScaledCapsuleHalfHeight();
		mMoveTargetTransform.SetLocation(TargetLocation);
	}

	// 루트가 Movable이 아니면 SetActorLocation이 실패하므로 보정 (BP 설정 실수 방어)
	if (RootComponent != nullptr && RootComponent->Mobility != EComponentMobility::Movable)
	{
		RootComponent->SetMobility(EComponentMobility::Movable);
	}

	// 틱 시작
	SetActorTickEnabled(true);
}

void AUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 이동스텝 연출 중이 아니면 틱 끔
	if (mMoveBarrier.IsValid() == false)
	{
		SetActorTickEnabled(false);
		return;
	}

	// 위치: 현재벡터를, 현재위치에서 다음위치로 지정한 속도만큼 이동한 값 계산
	const FVector NewLocation = FMath::VInterpConstantTo(
		GetActorLocation(),
		mMoveTargetTransform.GetLocation(),
		DeltaSeconds, mMoveSpeed);
	// 회전: 현재회전을, 다음 타일을 바라보도록 지정한 각도만큼 변경한 값 계산
	const FRotator NewRotation = FMath::RInterpConstantTo(
		GetActorRotation(),
		mMoveTargetTransform.Rotator(),
		DeltaSeconds, mRotationSpeed);
	SetActorLocationAndRotation(NewLocation, NewRotation);

	// 위치와 회전이 모두 목표와 일치하면 -> 해당 타일에 도착
	if (NewLocation.Equals(mMoveTargetTransform.GetLocation()) &&
		NewRotation.Equals(mMoveTargetTransform.Rotator()))
	{
		// 도착했으면 틱 해제
		SetActorTickEnabled(false);
		// 베리어 해제
		// -> MoveAction의 OnStepPresentationFinished() 호출
		// -> StartStep(NextTile) 해서 타일간 이동 반복
		mMoveBarrier.Reset();
	}
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