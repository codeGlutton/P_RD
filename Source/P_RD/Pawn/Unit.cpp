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
		// 방향 전환 연출 요청 구독
		mUnitModel->OnRotate.AddUObject(this, &AUnit::OnRotate);
		
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
		mUnitModel->OnRotate.RemoveAll(this);
	}
	mUnitModel.Reset();

	// 진행 중이던 이동스텝 연출이 있으면 배리어를 놓아서 완료로 처리
	mMoveBarrier.Reset();
	// 이동이 멈추면 틱도 비활성화
	SetActorTickEnabled(false);
	// 이동 중 해제될 수 있으므로 속도 상태도 초기화
	mCurrentMoveSpeed = 0.0f;
	mCurrentMoveVelocity = FVector::ZeroVector;
}

void AUnit::OnStartMoveStep(const FTileTransform& NextTileTransform, const FTransform& TargetWorldTransform, TSharedPtr<FPresentationBarrier> Barrier, float RemainingPathDistance)
{
	// 목표타일의 월드트랜스폼과 배리어 보관
	mMoveTargetTransform = TargetWorldTransform;
	mMoveBarrier = Barrier;
	// 목표 이후 남은 거리 저장 (제동거리 계산에 사용)
	mRemainingPathDistance = RemainingPathDistance;
	// @note 현재 속도(mCurrentMoveSpeed)는 초기화하지 않음 (중간에 멈칫하지 않게)

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

void AUnit::OnRotate(const FRotator& TargetWorldRotation, TSharedPtr<FPresentationBarrier> Barrier)
{
	// 목표를 "현재 위치 + 새 회전"으로 잡아서 Tick에서 처리하도록 설정
	mMoveTargetTransform = FTransform(TargetWorldRotation, GetActorLocation());
	// 인자로 들어온 배리어가 nullptr이면
	// -> Tick이 배리어를 요구하므로 빈 배리어를 만들어서 처리
	mMoveBarrier = Barrier.IsValid()
		? Barrier
		: FPresentationBarrier::Make(FOnFinishPresentation());
	// 제자리 회전이므로 남은 경로 거리는 0
	mRemainingPathDistance = 0.0f;

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

	// 목표 속도 결정
	// - 기본은 최대속도
	// - 제동거리에 들어가면 감속하면서 느려짐
	const FVector CurrentLocation = GetActorLocation();
	const float RemainingDistance = FVector::Dist(CurrentLocation, mMoveTargetTransform.GetLocation());

	// 남은 거리에서 멈출 수 있는 속도 계산
	float TargetSpeed = mMaxMoveSpeed;
	if (mDeceleration > 0.0f)
	{
		// 제동 기준 거리 = 이번 스텝 남은 거리 + 이후 경로 거리 (최종 목적지 기준으로 감속)
		// 목표가 많이 남으면 아래 BrakeSpeed가 최고속도보다 커지므로 최고속도로 순항하고
		// 목표가 가까워져서 제동거리 안에 들어가면 BrakeSpeed가 최고속도보다 낮아지므로 BrakeSpeed로 느려짐
		const float BrakeDistance = RemainingDistance + mRemainingPathDistance;
		// @details
		// 등가속도 공식은 `v^2 = v0^2 + 2as` 인데,
		// 최종속도 v=0 이 되면서 멈추는 거니까 이 공식은 `0 = v0^2 + 2as`
		// 이때 감속하니까 가속도는 -가 되고 2as를 좌변으로 넘기면 `2as = v0^2`
		// 따라서 `v0 = sqrt(2as)`
		const float BrakeSpeed = FMath::Sqrt(2.0f * mDeceleration * BrakeDistance);
		TargetSpeed = FMath::Min(TargetSpeed, BrakeSpeed);
	}

	// 현재 속도를 목표 속도로 가감속 (올릴 땐 가속도, 내릴 땐 감속도 사용)
	// 해당 가감속도가 0이면 즉시 목표 속도 도달
	const float InterpRate = (TargetSpeed < mCurrentMoveSpeed)
		? mDeceleration
		: mAcceleration;
	mCurrentMoveSpeed = (InterpRate > 0.0f)
		? FMath::FInterpConstantTo(mCurrentMoveSpeed, TargetSpeed, DeltaSeconds, InterpRate)
		: TargetSpeed;

	// 위치: 현재벡터를, 현재위치에서 다음위치로 현재 속도만큼 이동한 값 계산
	const FVector NewLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		mMoveTargetTransform.GetLocation(),
		DeltaSeconds, mCurrentMoveSpeed);
	// 회전: 현재회전을, 다음 타일을 바라보도록 지정한 각도만큼 변경한 값 계산
	const FRotator NewRotation = FMath::RInterpConstantTo(
		GetActorRotation(),
		mMoveTargetTransform.Rotator(),
		DeltaSeconds, mRotationSpeed);
	SetActorLocationAndRotation(NewLocation, NewRotation);

	// 현재 속도 저장
	// 애니메이션이 GetVelocity()로 읽어서 활용 가능
	mCurrentMoveVelocity
		= (DeltaSeconds > 0.0f)  // 혹시 0으로 나누는 경우를 방지
		? (NewLocation - CurrentLocation) / DeltaSeconds
		: FVector::ZeroVector;

	// 위치와 회전이 모두 목표와 일치하면 -> 해당 타일에 도착
	if (NewLocation.Equals(mMoveTargetTransform.GetLocation()) &&
		NewRotation.Equals(mMoveTargetTransform.Rotator()))
	{
		// 최종 목적지 도착이면 정지 상태로 초기화 (다음 이동은 0부터 다시 가속)
		if (mRemainingPathDistance <= KINDA_SMALL_NUMBER)
		{
			mCurrentMoveSpeed = 0.0f;
			mCurrentMoveVelocity = FVector::ZeroVector;
		}
		// 도착했으면 틱 해제
		SetActorTickEnabled(false);
		// 베리어 해제
		// -> MoveAction의 OnStepPresentationFinished() 호출
		// -> StartStep(NextTile) 해서 타일간 이동 반복
		mMoveBarrier.Reset();
	}
}

FVector AUnit::GetVelocity() const
{
	// 이동을 무브먼트컴포넌트 없이 직접 하므로
	// Tick()에서 계산해서 저장한 현재 속도를 반환
	return mCurrentMoveVelocity;
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