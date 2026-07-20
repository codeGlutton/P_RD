#include "Pawn/Unit.h"
#include "Pawn/UnitModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"

#include "RDCollision.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"

#include "Animation/BoardActorAnimInstance.h"
#include "Animation/Notify/EventTriggerPayload.h"

#include "NiagaraFunctionLibrary.h"

#include "Algo/BinarySearch.h"

namespace
{
	constexpr float FastMoveSpeedMultiplier = 2.85f;
	constexpr float FastMoveAccelerationMultiplier = 7.0f;
	constexpr float FastMoveRotationMultiplier = 3.0f;
	constexpr float FastMoveMinimumSpeed = 1500.0f;
	constexpr float FastMoveMinimumAcceleration = 7000.0f;
	constexpr float FastMoveMinimumDeceleration = 4600.0f;
	constexpr float FastMoveMinimumRotationSpeed = 1080.0f;

	float EaseOutCubic(float Alpha)
	{
		const float OneMinusAlpha = 1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - OneMinusAlpha * OneMinusAlpha * OneMinusAlpha;
	}

	float SmoothStep(float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return T * T * (3.0f - 2.0f * T);
	}

	float GetForcedMoveDistanceAlpha(float Alpha, EForcedMovePresentationType PresentationType)
	{
		switch (PresentationType)
		{
		case EForcedMovePresentationType::Pull:
		case EForcedMovePresentationType::Swap:
			return SmoothStep(Alpha);
		case EForcedMovePresentationType::Throw:
			return 1.0f - FMath::Square(1.0f - FMath::Clamp(Alpha, 0.0f, 1.0f));
		case EForcedMovePresentationType::Charge:
			// 돌진은 입력 직후 바로 속도가 붙되 접촉 직전에는 읽을 수 있을 만큼 감속한다.
			return EaseOutCubic(Alpha);
		case EForcedMovePresentationType::Leap:
			return SmoothStep(Alpha);
		case EForcedMovePresentationType::Push:
		default:
			return EaseOutCubic(Alpha);
		}
	}
}

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
	mArrowComp = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComp"));

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

	if (mArrowComp != nullptr)
	{
		mArrowComp->ArrowColor = FColor(150, 200, 255);
		mArrowComp->bTreatAsASprite = true;
		mArrowComp->SetupAttachment(mCapsuleComp);
		mArrowComp->bIsScreenSizeScaled = true;
		mArrowComp->SetSimulatePhysics(false);
	}
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
		// 위치 가져오기 구독
		mUnitModel->OnGetBoardActorWorldTransform.BindUObject(this, &AUnit::GetActorTransform);

		// 초기 배치 연출 요청 구독
		mUnitModel->OnPlaceTileTransform.AddUObject(this, &AUnit::OnPlaceTileTransform);

		// 이동 경로 통지 구독 (코너링을 포함한 폴리라인 생성용)
		mUnitModel->OnStartMovePath.AddUObject(this, &AUnit::OnStartMovePath);
		// 밀치기 경로 통지 구독 (일반 보행과 분리된 빠른 강제 이동 연출용)
		mUnitModel->OnStartForcedMovePath.AddUObject(this, &AUnit::OnStartForcedMovePath);
		// 충돌 순간의 압축/히트스톱/반동 통지 구독
		mUnitModel->OnPlayImpactPresentation.AddUObject(this, &AUnit::OnPlayImpactPresentation);
		// 이동 연출 요청 구독
		mUnitModel->OnStartMoveStep.AddUObject(this, &AUnit::OnStartMoveStep);
		// 방향 전환 연출 요청 구독
		mUnitModel->OnRotate.AddUObject(this, &AUnit::OnRotate);

		// 스킬 Effect 타격 연출 구독
		mUnitModel->OnPlayApplyAnimationUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> MotionEndBarrier, FOnRequestReceiveAnimation TriggerCallback, FGameplayTag ApplyMotionTag, ETileActorDirection LocalDirection) {
			if (mMeshComp != nullptr)
			{
				UBoardActorAnimInstance* BoardActorAnimInst = Cast<UBoardActorAnimInstance>(mMeshComp->GetAnimInstance());
				if (BoardActorAnimInst != nullptr)
				{
					/* 스킬 연출 유지를 위한 Barrier */

					FOnTriggerEndAnimationEvent OnTriggerEndAnimationEvent;
					OnTriggerEndAnimationEvent.AddLambda([MotionEndBarrier, TriggerCallback](FGameplayTag Tag, ETileActorDirection LocalDir, UAnimMontage* EndAnim, bool IsInterrupted) {
						TriggerCallback.ExecuteIfBound(nullptr);
						});
					BoardActorAnimInst->PlayMontageUsingTag(ApplyMotionTag, LocalDirection, MoveTemp(OnTriggerEndAnimationEvent));

					/* 피격 적용 대기를 위한 Barrier */

					FBoardActorAnimationEvent HitEvent;
					HitEvent.OnTriggerAnimationEvent.AddLambda([TriggerCallback](FGameplayTag Tag, ETileActorDirection LocalDir, UAnimMontage* EndAnim, const FEventTriggerPayload* Payload) {
						const FApplyEventTriggerPayload* ApplyPayload = nullptr;
						if (Payload != nullptr && Payload->GetScriptStruct() == FApplyEventTriggerPayload::StaticStruct())
						{
							ApplyPayload = reinterpret_cast<const FApplyEventTriggerPayload*>(Payload);
						}
						TriggerCallback.ExecuteIfBound(ApplyPayload);
						});
					HitEvent.mIsOneTimeEvent = true;
					BoardActorAnimInst->RegisterTagEventOnMontage(AnimationTags::Animation_Event_Hit, MoveTemp(HitEvent));
				}
			}
			});

		// 스킬 Effect 피격 연출 구독
		mUnitModel->OnPlayReceiveAnimationUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> MotionEndBarrier, const FApplyEventTriggerPayload* Payload, FGameplayTag ReceiveMotionTag, ETileActorDirection LocalDirection) {
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
				if (Payload != nullptr)
				{
					for (const FApplyNiagaraSpawnData& NiagaraSpawnData : Payload->mNiagaraSpawnDatas)
					{
						SpawnHitVFX(NiagaraSpawnData, LocalDirection);
					}
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
		mUnitModel->OnStartMovePath.RemoveAll(this);
		mUnitModel->OnStartForcedMovePath.RemoveAll(this);
		mUnitModel->OnPlayImpactPresentation.RemoveAll(this);
		mUnitModel->OnStartMoveStep.RemoveAll(this);
		mUnitModel->OnRotate.RemoveAll(this);
	}
	mUnitModel.Reset();

	// 배리어 해제 콜백보다 먼저 메시를 원래 상대 트랜스폼으로 되돌린다.
	ResetImpactPresentation();
	ResetForcedMovePresentation();
	// 진행 중이던 이동스텝 연출이 있으면 배리어를 놓아서 완료로 처리
	mMoveBarrier.Reset();
	// 이동이 멈추면 틱도 비활성화
	SetActorTickEnabled(false);
	// 이동 중 해제될 수 있으므로 속도 상태도 초기화
	mCurrentMoveSpeed = 0.0f;
	mCurrentMoveVelocity = FVector::ZeroVector;
	// 이동 중 해제될 수 있으므로 폴리라인 이동상태도 초기화
	ResetPolyLineState();
}

void AUnit::OnPlaceTileTransform(const FTileTransform& TileTransform, const FTransform& Transform)
{
	FVector UnitLocation = Transform.GetLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FTransform UnitTransform = Transform;
	UnitTransform.SetLocation(UnitLocation);

	SetActorTransform(UnitTransform);
}

void AUnit::OnStartMovePath(const TArray<FVector>& PathWorldLocations)
{
	// 새 일반 경로가 시작되면 이전 강제 이동의 메시 오프셋이 남지 않게 한다.
	ResetForcedMovePresentation();

	// 경로 전체를 코너링 곡선을 포함한 폴리라인으로 재구성
	BakePolyLinePoints(PathWorldLocations, mCornerCutRatio, mCornerTension, mPolyLinePoints, mPolyLineDistances, mStepMarkerDistances);

	// 진행 상태 초기화
	mCurrentMoveStep = 0;
	mPolyLineTraveledDistance = 0.0f;
}

void AUnit::OnStartForcedMovePath(
	const TArray<FVector>& PathWorldLocations,
	EForcedMovePresentationType PresentationType)
{
	// 경로 베이크와 스텝 마커는 일반 이동과 공유한다. 이후 Tick 경로만 강제 이동 전용으로 분기한다.
	OnStartMovePath(PathWorldLocations);
	if (PathWorldLocations.Num() < 2 || mPolyLinePoints.Num() < 2)
	{
		return;
	}

	const int32 MoveTileCount = PathWorldLocations.Num() - 1;
	const float DistanceAlpha = FMath::Clamp(StaticCast<float>(MoveTileCount - 1) / 4.0f, 0.0f, 1.0f);
	float FirstTileDuration = 0.13f;
	float AdditionalTileDuration = 0.05f;
	float MaxTravelDuration = 0.38f;
	float SettleDuration = 0.06f;
	switch (PresentationType)
	{
	case EForcedMovePresentationType::Pull:
		FirstTileDuration = 0.18f;
		AdditionalTileDuration = 0.07f;
		MaxTravelDuration = 0.48f;
		SettleDuration = 0.09f;
		break;
	case EForcedMovePresentationType::Throw:
		FirstTileDuration = 0.25f;
		AdditionalTileDuration = 0.08f;
		MaxTravelDuration = 0.58f;
		SettleDuration = 0.10f;
		break;
	case EForcedMovePresentationType::Swap:
		FirstTileDuration = 0.16f;
		AdditionalTileDuration = 0.05f;
		MaxTravelDuration = 0.32f;
		SettleDuration = 0.07f;
		break;
	case EForcedMovePresentationType::Charge:
		FirstTileDuration = 0.12f;
		AdditionalTileDuration = 0.04f;
		MaxTravelDuration = 0.34f;
		SettleDuration = 0.05f;
		break;
	case EForcedMovePresentationType::Leap:
		FirstTileDuration = 0.28f;
		AdditionalTileDuration = 0.0f;
		MaxTravelDuration = 0.28f;
		SettleDuration = 0.07f;
		break;
	case EForcedMovePresentationType::Push:
	default:
		break;
	}
	const float TravelDuration = FMath::Min(
		FirstTileDuration
			+ AdditionalTileDuration * StaticCast<float>(FMath::Max(MoveTileCount - 1, 0)),
		MaxTravelDuration);

	mIsForcedMovePresentation = true;
	mForcedMovePresentationType = PresentationType;
	mForcedMoveElapsed = 0.0f;
	mForcedMoveTravelDuration = FMath::Max(TravelDuration, KINDA_SMALL_NUMBER);
	mForcedMoveDuration = mForcedMoveTravelDuration + SettleDuration;
	mForcedMoveArcHeight = 14.0f;
	mForcedMoveOvershootDistance = 26.0f;
	mForcedMoveLateralDistance = 0.0f;
	mForcedMoveTiltDegrees = 17.0f;
	mForcedMoveSpinDegrees = 0.0f;
	switch (PresentationType)
	{
	case EForcedMovePresentationType::Pull:
		mForcedMoveArcHeight = FMath::Lerp(3.0f, 7.0f, DistanceAlpha);
		mForcedMoveOvershootDistance = FMath::Lerp(10.0f, 18.0f, DistanceAlpha);
		mForcedMoveLateralDistance = FMath::Lerp(4.0f, 8.0f, DistanceAlpha);
		mForcedMoveTiltDegrees = -18.0f;
		break;
	case EForcedMovePresentationType::Throw:
		mForcedMoveArcHeight = FMath::Lerp(82.0f, 145.0f, DistanceAlpha);
		mForcedMoveOvershootDistance = FMath::Lerp(30.0f, 48.0f, DistanceAlpha);
		mForcedMoveTiltDegrees = 9.0f;
		mForcedMoveSpinDegrees = MoveTileCount >= 3 ? 720.0f : 360.0f;
		break;
	case EForcedMovePresentationType::Swap:
		mForcedMoveArcHeight = 24.0f;
		mForcedMoveOvershootDistance = 8.0f;
		mForcedMoveLateralDistance = 34.0f;
		mForcedMoveTiltDegrees = 16.0f;
		break;
	case EForcedMovePresentationType::Charge:
		mForcedMoveArcHeight = FMath::Lerp(5.0f, 10.0f, DistanceAlpha);
		mForcedMoveOvershootDistance = 14.0f;
		mForcedMoveTiltDegrees = 21.0f;
		break;
	case EForcedMovePresentationType::Leap:
		mForcedMoveArcHeight = FMath::Lerp(105.0f, 155.0f, DistanceAlpha);
		mForcedMoveOvershootDistance = 7.0f;
		mForcedMoveTiltDegrees = -11.0f;
		break;
	case EForcedMovePresentationType::Push:
	default:
		mForcedMoveArcHeight = FMath::Lerp(12.0f, 22.0f, DistanceAlpha);
		mForcedMoveOvershootDistance = FMath::Lerp(24.0f, 34.0f, DistanceAlpha);
		mForcedMoveTiltDegrees = FMath::Lerp(18.0f, 27.0f, DistanceAlpha);
		break;
	}
	mForcedMoveWorldDirection = PathWorldLocations.Last() - PathWorldLocations[0];
	mForcedMoveWorldDirection.Z = 0.0f;
	mForcedMoveWorldDirection = mForcedMoveWorldDirection.GetSafeNormal();
	mForcedMoveFacingRotation = GetActorRotation();
	mCurrentMoveSpeed = 0.0f;
	mCurrentMoveVelocity = FVector::ZeroVector;

	if (mMeshComp != nullptr)
	{
		mForcedMoveBaseMeshRelativeTransform = mMeshComp->GetRelativeTransform();
	}
}

void AUnit::OnPlayImpactPresentation(
	const FVector& WorldDirection,
	float Strength,
	EImpactPresentationType PresentationType)
{
	if (mMeshComp == nullptr)
	{
		return;
	}

	ResetImpactPresentation();
	mIsImpactPresentation = true;
	mImpactPresentationElapsed = 0.0f;
	mImpactPresentationStrength = FMath::Clamp(Strength, 0.65f, 1.65f);
	mImpactPresentationType = PresentationType;
	mImpactWorldDirection = WorldDirection;
	mImpactWorldDirection.Z = 0.0f;
	mImpactWorldDirection = mImpactWorldDirection.GetSafeNormal();
	if (mImpactWorldDirection.IsNearlyZero())
	{
		mImpactWorldDirection = GetActorForwardVector();
	}
	mImpactPresentationDuration = PresentationType == EImpactPresentationType::ChargeContact
		? 0.38f
		: (PresentationType == EImpactPresentationType::Receiver ? 0.26f : 0.24f);
	mImpactBaseMeshRelativeTransform = mIsForcedMovePresentation
		? mForcedMoveBaseMeshRelativeTransform
		: mMeshComp->GetRelativeTransform();
	mMeshComp->SetRelativeTransform(mImpactBaseMeshRelativeTransform);
	SetActorTickEnabled(true);
}

void AUnit::OnStartMoveStep(const FTileTransform& NextTileTransform, const FTransform& TargetWorldTransform, TSharedPtr<FPresentationBarrier> Barrier, float RemainingPathDistance)
{
	// 돌진 접촉 반동 도중 다음 이동이 재개되면 잔여 반동을 즉시 정리하고 같은 경로를 이어간다.
	ResetImpactPresentation();
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

	// 폴리라인 이동모드면 경로 순번 진행
	if (mPolyLinePoints.Num() >= 2)
	{
	    // 현재 스텝(타일) 증가 -> 다음 타일 관련된 작업 진행
	    // @note 타일 하나에 여러 개의 점들이 포함될 수 있으므로 기준 타일을 설정하고 그 안에서 작업 진행
		++mCurrentMoveStep;
		// 경로 통지와 스텝 수가 어긋나면 폴리라인 이동을 포기하고 직선 이동모드로 폴백
		if (mStepMarkerDistances.IsValidIndex(mCurrentMoveStep) == false)
		{
			ResetPolyLineState();
		}
	}

	// 틱 시작
	SetActorTickEnabled(true);
}

void AUnit::OnRotate(const FRotator& TargetWorldRotation, TSharedPtr<FPresentationBarrier> Barrier)
{
	// 제자리 회전은 직선이동모드로 처리하므로 폴리라인이동모드는 리셋
	ResetPolyLineState();

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

void AUnit::SpawnHitVFX(const FApplyNiagaraSpawnData& NiagaraSpawnData, ETileActorDirection LocalDirection) const
{
	if (NiagaraSpawnData.mNiagaraSystem == nullptr)
	{
		return;
	}

	// 소켓의 컴포넌트 기준 Transform
	const FTransform SocketLocalTransform = GetMesh()->GetSocketTransform(NiagaraSpawnData.mSocketName, RTS_Component);

	// 추가 방향 절대 회전 Transform
	const float Yaw = StaticCast<uint8>(LocalDirection) * 90.f;
	const FTransform DirectionTransform(FRotator(0.f, Yaw, 0.f).Quaternion());

	// 소켓 Transform을 컴포넌트 축으로 절대 회전 방향으로 돌리고, 그 다음에 로컬 트랜스폼 적용
	const FTransform FinalLocalTransform = NiagaraSpawnData.mRelativeTransform * DirectionTransform * SocketLocalTransform;
	// 월드 트랜스폼으로 변환
	const FTransform FinalWorldTransform = FinalLocalTransform * GetMesh()->GetComponentTransform();

	if (NiagaraSpawnData.mAttached == true)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSpawnData.mNiagaraSystem,
			GetMesh(),
			NAME_None,
			FinalLocalTransform.GetLocation(),
			FinalLocalTransform.Rotator(),
			FinalLocalTransform.GetScale3D(),
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::None
		);
	}
	else
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			NiagaraSpawnData.mNiagaraSystem,
			FinalWorldTransform.GetLocation(),
			FinalWorldTransform.Rotator(),
			FinalWorldTransform.GetScale3D()
		);
	}
}

void AUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 돌진이 적을 미는 동안 기사는 타일 경로를 잠시 멈추고 접촉 히트스톱만 재생한다.
	if (mMoveBarrier.IsValid() == false && mIsImpactPresentation)
	{
		TickImpactPresentation(DeltaSeconds);
		return;
	}

	// 이동스텝/충돌 연출 중이 아니면 틱 끔
	if (mMoveBarrier.IsValid() == false)
	{
		ResetForcedMovePresentation();
		SetActorTickEnabled(false);
		return;
	}

	// 폴리라인이 있으면 폴리라인이동모드로 처리
	if (mPolyLinePoints.Num() >= 2)
	{
		if (mIsForcedMovePresentation)
		{
			TickForcedMove(DeltaSeconds);
		}
		else
		{
			TickPolyLine(DeltaSeconds);
		}
		return;
	}

	// 목표 속도 결정
	// - 기본은 최대속도
	// - 제동거리에 들어가면 감속하면서 느려짐
	const FVector CurrentLocation = GetActorLocation();
	const float RemainingDistance = FVector::Dist(CurrentLocation, mMoveTargetTransform.GetLocation());

	// 남은 거리에서 멈출 수 있는 속도 계산
	const float EffectiveMaxSpeed = FMath::Max(mMaxMoveSpeed * FastMoveSpeedMultiplier, FastMoveMinimumSpeed);
	const float EffectiveAcceleration = FMath::Max(
		mAcceleration * FastMoveAccelerationMultiplier,
		FastMoveMinimumAcceleration);
	const float EffectiveDeceleration = FMath::Max(
		mDeceleration * FastMoveAccelerationMultiplier,
		FastMoveMinimumDeceleration);
	const float EffectiveRotationSpeed = FMath::Max(
		mRotationSpeed * FastMoveRotationMultiplier,
		FastMoveMinimumRotationSpeed);
	float TargetSpeed = EffectiveMaxSpeed;
	if (EffectiveDeceleration > 0.0f)
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
		const float BrakeSpeed = FMath::Sqrt(2.0f * EffectiveDeceleration * BrakeDistance);
		TargetSpeed = FMath::Min(TargetSpeed, BrakeSpeed);
	}

	// 현재 속도를 목표 속도로 가감속 (올릴 땐 가속도, 내릴 땐 감속도 사용)
	// 해당 가감속도가 0이면 즉시 목표 속도 도달
	const float InterpRate = (TargetSpeed < mCurrentMoveSpeed)
		? EffectiveDeceleration
		: EffectiveAcceleration;
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
		DeltaSeconds, EffectiveRotationSpeed);
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

void AUnit::TickForcedMove(float DeltaSeconds)
{
	if (mPolyLinePoints.Num() < 2
		|| mPolyLineDistances.Num() != mPolyLinePoints.Num()
		|| mStepMarkerDistances.Num() < 2
		|| mForcedMoveDuration <= KINDA_SMALL_NUMBER)
	{
		// 비정상 경로에서는 메시를 즉시 원복하고 기존 직선 이동 폴백이 배리어를 마무리하게 한다.
		ResetPolyLineState();
		return;
	}

	const float TotalDistance = mPolyLineDistances.Last();
	mForcedMoveElapsed = FMath::Min(
		mForcedMoveElapsed + FMath::Max(DeltaSeconds, 0.0f),
		mForcedMoveDuration);

	const float TravelAlpha = FMath::Clamp(
		mForcedMoveElapsed / mForcedMoveTravelDuration,
		0.0f,
		1.0f);
	mPolyLineTraveledDistance = TotalDistance * GetForcedMoveDistanceAlpha(
		TravelAlpha,
		mForcedMovePresentationType);

	FVector SampleLocation = FVector::ZeroVector;
	FVector SampleTangent = FVector::ZeroVector;
	if (GetPolyLinePoint(mPolyLineTraveledDistance, SampleLocation, SampleTangent) == false)
	{
		ResetPolyLineState();
		return;
	}

	// 폴리라인은 타일 바닥 기준이므로 루트는 기존 이동과 똑같이 캡슐 반높이만큼 보정한다.
	if (mCapsuleComp != nullptr)
	{
		SampleLocation.Z += mCapsuleComp->GetScaledCapsuleHalfHeight();
	}

	// 밀려나는 동안 이동 방향으로 몸을 돌리지 않는다. 보행 속도도 노출하지 않아 Walk Blend를 억제한다.
	SetActorLocationAndRotation(SampleLocation, mForcedMoveFacingRotation);
	mCurrentMoveSpeed = 0.0f;
	mCurrentMoveVelocity = FVector::ZeroVector;

	const bool bSettling = mForcedMoveElapsed >= mForcedMoveTravelDuration;
	const float SettleAlpha = bSettling
		? FMath::Clamp(
			(mForcedMoveElapsed - mForcedMoveTravelDuration)
			/ FMath::Max(mForcedMoveDuration - mForcedMoveTravelDuration, KINDA_SMALL_NUMBER),
			0.0f,
			1.0f)
		: -1.0f;
	ApplyForcedMoveMeshPresentation(TravelAlpha, SettleAlpha);

	// 논리 이동은 기존과 동일하게 각 타일 마커에서 배리어를 놓아 다음 StartDicePushStep을 진행한다.
	// 프레임 드롭으로 여러 마커를 한 번에 지난 경우도 모두 처리한다.
	while (mMoveBarrier.IsValid()
		&& mCurrentMoveStep < mStepMarkerDistances.Num() - 1
		&& mPolyLineTraveledDistance >= mStepMarkerDistances[mCurrentMoveStep])
	{
		mMoveBarrier.Reset();
	}

	// 배리어 콜백 중 경로가 취소/리셋될 수 있다.
	if (mPolyLinePoints.Num() < 2)
	{
		return;
	}

	// 루트가 목적지에 닿은 뒤 짧은 메시 반동까지 끝나야 마지막 배리어를 놓는다.
	if (mMoveBarrier.IsValid()
		&& mCurrentMoveStep >= mStepMarkerDistances.Num() - 1
		&& mPolyLineTraveledDistance >= TotalDistance - KINDA_SMALL_NUMBER
		&& mForcedMoveElapsed >= mForcedMoveDuration - KINDA_SMALL_NUMBER)
	{
		mCurrentMoveSpeed = 0.0f;
		mCurrentMoveVelocity = FVector::ZeroVector;
		ResetPolyLineState();
		SetActorTickEnabled(false);
		mMoveBarrier.Reset();
	}
}

void AUnit::ApplyForcedMoveMeshPresentation(float TravelAlpha, float SettleAlpha)
{
	if (mIsForcedMovePresentation == false || mMeshComp == nullptr)
	{
		return;
	}

	const float ClampedTravelAlpha = FMath::Clamp(TravelAlpha, 0.0f, 1.0f);
	const float FlightWave = FMath::Sin(PI * ClampedTravelAlpha);
	float ArcOffset = FlightWave * mForcedMoveArcHeight;
	float LateralOffset = mForcedMoveLateralDistance * FlightWave;
	if (mForcedMovePresentationType == EForcedMovePresentationType::Pull)
	{
		// 당기기는 공중제비 대신 지면을 긁으며 좌우로 한 번 흔들린다.
		LateralOffset *= FMath::Sin(3.0f * PI * ClampedTravelAlpha);
	}

	float OvershootAlpha = 0.0f;
	float ImpactStrength = 0.0f;
	float LandingBounce = 0.0f;
	if (SettleAlpha < 0.0f)
	{
		// 이동 후반부에만 메시가 루트보다 조금 앞서 나가도록 해 도착 충격을 준비한다.
		OvershootAlpha = FMath::Pow(ClampedTravelAlpha, 4.0f);
	}
	else
	{
		const float SettleEase = EaseOutCubic(SettleAlpha);
		OvershootAlpha = 1.0f - SettleEase;
		ImpactStrength = 1.0f - SettleEase;
		if (mForcedMovePresentationType == EForcedMovePresentationType::Throw
			|| mForcedMovePresentationType == EForcedMovePresentationType::Leap)
		{
			LandingBounce = FMath::Sin(PI * SettleAlpha)
				* (1.0f - SettleAlpha)
				* (mForcedMovePresentationType == EForcedMovePresentationType::Leap ? 9.0f : 16.0f);
		}
		else if (mForcedMovePresentationType == EForcedMovePresentationType::Push)
		{
			LandingBounce = FMath::Sin(PI * SettleAlpha)
				* (1.0f - SettleAlpha) * 5.0f;
		}
	}

	const FVector WorldLateral = FVector::CrossProduct(
		FVector::UpVector,
		mForcedMoveWorldDirection).GetSafeNormal();
	const FVector WorldOffset = mForcedMoveWorldDirection
		* (mForcedMoveOvershootDistance * OvershootAlpha)
		+ WorldLateral * LateralOffset
		+ FVector::UpVector * (ArcOffset + LandingBounce);
	const FVector RelativeOffset = GetActorTransform().InverseTransformVectorNoScale(WorldOffset);

	FTransform MeshTransform = mForcedMoveBaseMeshRelativeTransform;
	MeshTransform.SetLocation(MeshTransform.GetLocation() + RelativeOffset);

	const FVector WorldTumbleAxis = FVector::CrossProduct(
		mForcedMoveWorldDirection,
		FVector::UpVector).GetSafeNormal();
	FVector LocalTumbleAxis = GetActorTransform().InverseTransformVectorNoScale(WorldTumbleAxis).GetSafeNormal();
	if (LocalTumbleAxis.IsNearlyZero())
	{
		LocalTumbleAxis = FVector::RightVector;
	}
	float TiltDegrees = FlightWave * mForcedMoveTiltDegrees;
	float SpinDegrees = 0.0f;
	if (mForcedMovePresentationType == EForcedMovePresentationType::Throw)
	{
		SpinDegrees = mForcedMoveSpinDegrees * ClampedTravelAlpha;
	}
	else if (mForcedMovePresentationType == EForcedMovePresentationType::Swap)
	{
		// 서로 반대 방향으로 같은 규칙을 적용하면 두 유닛이 서로 다른 측면으로 비켜 지나간다.
		TiltDegrees = FlightWave * mForcedMoveTiltDegrees;
	}
	else if (mForcedMovePresentationType == EForcedMovePresentationType::Charge)
	{
		TiltDegrees = -FlightWave * mForcedMoveTiltDegrees;
	}
	else if (mForcedMovePresentationType == EForcedMovePresentationType::Leap)
	{
		TiltDegrees = FlightWave * mForcedMoveTiltDegrees;
	}
	const FQuat BodyRotation(LocalTumbleAxis, FMath::DegreesToRadians(TiltDegrees + SpinDegrees));
	MeshTransform.SetRotation(BodyRotation * mForcedMoveBaseMeshRelativeTransform.GetRotation());

	const FVector BaseScale = mForcedMoveBaseMeshRelativeTransform.GetScale3D();
	const float ImpactScale = mForcedMovePresentationType == EForcedMovePresentationType::Throw
		? 1.75f
		: (mForcedMovePresentationType == EForcedMovePresentationType::Pull ? 1.30f : 1.0f);
	const float PullTension = mForcedMovePresentationType == EForcedMovePresentationType::Pull
		? FMath::Sin(PI * ClampedTravelAlpha)
		: 0.0f;
	const float AirStretch = mForcedMovePresentationType == EForcedMovePresentationType::Throw
		? FlightWave
		: 0.0f;
	const float ChargeCompression = mForcedMovePresentationType == EForcedMovePresentationType::Charge
		? 0.04f * FlightWave
		: 0.0f;
	const FVector SquashMultiplier(
		1.0f + 0.04f * ImpactStrength * ImpactScale + 0.02f * PullTension - 0.035f * AirStretch + ChargeCompression,
		1.0f + 0.04f * ImpactStrength * ImpactScale + 0.02f * PullTension - 0.035f * AirStretch + ChargeCompression,
		1.0f - 0.08f * ImpactStrength * ImpactScale + 0.055f * AirStretch - 0.04f * PullTension - ChargeCompression);
	MeshTransform.SetScale3D(BaseScale * SquashMultiplier);
	mMeshComp->SetRelativeTransform(MeshTransform);
}

void AUnit::TickImpactPresentation(float DeltaSeconds)
{
	if (mIsImpactPresentation == false || mMeshComp == nullptr)
	{
		return;
	}

	mImpactPresentationElapsed = FMath::Min(
		mImpactPresentationElapsed + FMath::Max(DeltaSeconds, 0.0f),
		mImpactPresentationDuration);
	const float Alpha = FMath::Clamp(
		mImpactPresentationElapsed / FMath::Max(mImpactPresentationDuration, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f);

	float DirectionalOffset = 0.0f;
	float TiltDegrees = 0.0f;
	float Squash = 0.0f;
	if (mImpactPresentationType == EImpactPresentationType::ChargeContact)
	{
		// 10% 진입 -> 짧은 정지 -> 반대 방향 반동. 적이 날아가는 동안 기사 몸은 접촉점에 남는다.
		if (Alpha < 0.10f)
		{
			const float ContactAlpha = SmoothStep(Alpha / 0.10f);
			DirectionalOffset = FMath::Lerp(0.0f, 11.0f, ContactAlpha);
			TiltDegrees = FMath::Lerp(0.0f, -16.0f, ContactAlpha);
			Squash = ContactAlpha;
		}
		else if (Alpha < 0.30f)
		{
			DirectionalOffset = 11.0f;
			TiltDegrees = -16.0f;
			Squash = 1.0f;
		}
		else
		{
			const float RecoverAlpha = (Alpha - 0.30f) / 0.70f;
			const float RecoilWave = FMath::Sin(PI * RecoverAlpha) * (1.0f - RecoverAlpha);
			DirectionalOffset = FMath::Lerp(11.0f, 0.0f, EaseOutCubic(RecoverAlpha)) - 12.0f * RecoilWave;
			TiltDegrees = FMath::Lerp(-16.0f, 0.0f, EaseOutCubic(RecoverAlpha)) + 10.0f * RecoilWave;
			Squash = 1.0f - EaseOutCubic(RecoverAlpha);
		}
	}
	else
	{
		const float Wave = FMath::Sin(PI * Alpha);
		const float Fade = 1.0f - Alpha;
		const bool bReceiver = mImpactPresentationType == EImpactPresentationType::Receiver;
		DirectionalOffset = (bReceiver ? 18.0f : -13.0f) * Wave * Fade;
		TiltDegrees = (bReceiver ? -24.0f : 18.0f) * Wave;
		Squash = FMath::Pow(1.0f - Alpha, 3.0f);
	}
	DirectionalOffset *= mImpactPresentationStrength;
	TiltDegrees *= mImpactPresentationStrength;

	const FVector WorldOffset = mImpactWorldDirection * DirectionalOffset;
	const FVector RelativeOffset = GetActorTransform().InverseTransformVectorNoScale(WorldOffset);
	FTransform MeshTransform = mImpactBaseMeshRelativeTransform;
	MeshTransform.SetLocation(MeshTransform.GetLocation() + RelativeOffset);
	FVector WorldTiltAxis = FVector::CrossProduct(mImpactWorldDirection, FVector::UpVector).GetSafeNormal();
	FVector LocalTiltAxis = GetActorTransform().InverseTransformVectorNoScale(WorldTiltAxis).GetSafeNormal();
	if (LocalTiltAxis.IsNearlyZero())
	{
		LocalTiltAxis = FVector::RightVector;
	}
	MeshTransform.SetRotation(
		FQuat(LocalTiltAxis, FMath::DegreesToRadians(TiltDegrees))
		* mImpactBaseMeshRelativeTransform.GetRotation());
	const FVector BaseScale = mImpactBaseMeshRelativeTransform.GetScale3D();
	MeshTransform.SetScale3D(BaseScale * FVector(
		1.0f + 0.055f * Squash,
		1.0f + 0.055f * Squash,
		1.0f - 0.10f * Squash));
	mMeshComp->SetRelativeTransform(MeshTransform);

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		ResetImpactPresentation();
		if (mMoveBarrier.IsValid() == false)
		{
			ResetPolyLineState();
			SetActorTickEnabled(false);
		}
	}
}

void AUnit::ResetForcedMovePresentation()
{
	if (mIsForcedMovePresentation && mMeshComp != nullptr)
	{
		mMeshComp->SetRelativeTransform(mForcedMoveBaseMeshRelativeTransform);
	}

	mIsForcedMovePresentation = false;
	mForcedMoveElapsed = 0.0f;
	mForcedMoveDuration = 0.0f;
	mForcedMoveTravelDuration = 0.0f;
	mForcedMoveArcHeight = 0.0f;
	mForcedMoveOvershootDistance = 0.0f;
	mForcedMoveLateralDistance = 0.0f;
	mForcedMoveTiltDegrees = 0.0f;
	mForcedMoveSpinDegrees = 0.0f;
	mForcedMovePresentationType = EForcedMovePresentationType::Push;
	mForcedMoveWorldDirection = FVector::ZeroVector;
	mForcedMoveFacingRotation = FRotator::ZeroRotator;
	mForcedMoveBaseMeshRelativeTransform = FTransform::Identity;
}

void AUnit::ResetImpactPresentation()
{
	if (mIsImpactPresentation && mMeshComp != nullptr)
	{
		mMeshComp->SetRelativeTransform(mImpactBaseMeshRelativeTransform);
	}
	mIsImpactPresentation = false;
	mImpactPresentationElapsed = 0.0f;
	mImpactPresentationDuration = 0.0f;
	mImpactPresentationStrength = 1.0f;
	mImpactPresentationType = EImpactPresentationType::Source;
	mImpactWorldDirection = FVector::ZeroVector;
	mImpactBaseMeshRelativeTransform = FTransform::Identity;
}

void AUnit::TickPolyLine(float DeltaSeconds)
{
	// 전체 길이와 남은 거리 (모든 거리는 폴리라인 시작점 기준)
	const float TotalDistance = mPolyLineDistances.Last();
	const float RemainingDistance = TotalDistance - mPolyLineTraveledDistance;

	// 남은 거리에서 멈출 수 있는 속도 계산 (직선이동모드와 동일한 등가속도 공식)
	const float EffectiveMaxSpeed = FMath::Max(mMaxMoveSpeed * FastMoveSpeedMultiplier, FastMoveMinimumSpeed);
	const float EffectiveAcceleration = FMath::Max(
		mAcceleration * FastMoveAccelerationMultiplier,
		FastMoveMinimumAcceleration);
	const float EffectiveDeceleration = FMath::Max(
		mDeceleration * FastMoveAccelerationMultiplier,
		FastMoveMinimumDeceleration);
	const float EffectiveRotationSpeed = FMath::Max(
		mRotationSpeed * FastMoveRotationMultiplier,
		FastMoveMinimumRotationSpeed);
	float TargetSpeed = EffectiveMaxSpeed;
	if (EffectiveDeceleration > 0.0f)
	{
		const float BrakeSpeed = FMath::Sqrt(2.0f * EffectiveDeceleration * RemainingDistance);
		TargetSpeed = FMath::Min(TargetSpeed, BrakeSpeed);
	}

	// 현재 속도를 목표 속도로 가감속 (올릴 땐 가속도, 내릴 땐 감속도 사용)
	// 해당 가감속도가 0이면 즉시 목표 속도 도달
	const float InterpRate = (TargetSpeed < mCurrentMoveSpeed)
		? EffectiveDeceleration
		: EffectiveAcceleration;
	mCurrentMoveSpeed = (InterpRate > 0.0f)
		? FMath::FInterpConstantTo(mCurrentMoveSpeed, TargetSpeed, DeltaSeconds, InterpRate)
		: TargetSpeed;

	/**
	 * @brief 진행거리 갱신
	 * @details
	 *   진행거리를 계산하는 이유는, 진행거리를 알면
	 *   -> 어느 지점을 지나고 있는 지 알 수 있고
	 *   -> 그 지점에서의 접선(바라보는 방향)을 알 수 있으므로
	 *   -> 보드액터를 '특정위치'에 '특정방향'을 바라보게 세울 수 있음
	 */
	mPolyLineTraveledDistance = FMath::Min(
	    mPolyLineTraveledDistance + mCurrentMoveSpeed * DeltaSeconds,
	    TotalDistance);

	// 진행거리에 해당하는 폴리라인 위치/접선 계산
	FVector SampleLocation = FVector::ZeroVector;
	FVector SampleTangent = FVector::ZeroVector;
	if (GetPolyLinePoint(mPolyLineTraveledDistance, SampleLocation, SampleTangent) == false)
	{
		// 폴리라인이 비정상이면 직선이동모드로 폴백
		ResetPolyLineState();
		return;
	}

	// 폴리라인은 타일 바닥 기준이므로 캡슐 반높이만큼 보정
	if (mCapsuleComp != nullptr)
	{
		SampleLocation.Z += mCapsuleComp->GetScaledCapsuleHalfHeight();
	}

	// 이 타일에서의 기본 회전값
	FRotator TargetRotation = mMoveTargetTransform.Rotator();
	FVector FlatTangent = SampleTangent;
    // 접선벡터에서 요 회전만 바꾸기 위해서 Z축은 0으로 설정
	FlatTangent.Z = 0.0f;
	if (RemainingDistance > KINDA_SMALL_NUMBER && FlatTangent.IsNearlyZero() == false)
	{
	    // 접선 벡터를 회전값으로 변환 -> 이 회전값만큼 액터가 회전한다
		TargetRotation = FlatTangent.Rotation();
	}

	// 위치와 회전 적용
	const FVector PreviousLocation = GetActorLocation();
	const FRotator NewRotation = FMath::RInterpConstantTo(
		GetActorRotation(),
		TargetRotation,
		DeltaSeconds,
		EffectiveRotationSpeed);
	SetActorLocationAndRotation(SampleLocation, NewRotation);

	// 현재 속도 저장
	// 애니메이션이 GetVelocity()로 읽어서 활용 가능
	mCurrentMoveVelocity = (DeltaSeconds > 0.0f)
		? (SampleLocation - PreviousLocation) / DeltaSeconds
		: FVector::ZeroVector;

	// 해당 타일을 지났는 지 판정
	// @note
	// FPS가 느리면 한 프레임에 여러 마커를 지날 수 있으므로 반복 처리
	while (mMoveBarrier.IsValid()
	    && mCurrentMoveStep < mStepMarkerDistances.Num() - 1
	    && mPolyLineTraveledDistance >= mStepMarkerDistances[mCurrentMoveStep])
	{
		mMoveBarrier.Reset();
	}

	// (이유는 몰라도) 폴리라인이 리셋됐으면 바로 종료
	if (mPolyLinePoints.Num() < 2)
	{
		return;
	}

	// 최종 지점 도착 + 회전 완료면 이동 종료
	if (mMoveBarrier.IsValid() &&
		mCurrentMoveStep >= mStepMarkerDistances.Num() - 1 &&
		mPolyLineTraveledDistance >= TotalDistance &&
		NewRotation.Equals(mMoveTargetTransform.Rotator()))
	{
		// 정지 상태로 초기화 (다음 이동은 0부터 다시 가속)
		mCurrentMoveSpeed = 0.0f;
		mCurrentMoveVelocity = FVector::ZeroVector;
		ResetPolyLineState();
		// 도착했으면 틱 해제
		SetActorTickEnabled(false);
		// 베리어 해제
		// -> MoveAction의 OnStepPresentationFinished() 호출
		// -> 마지막 스텝이므로 이동 완료 처리
		mMoveBarrier.Reset();
	}
}

bool AUnit::GetPolyLinePoint(float Distance, FVector& OutLocation, FVector& OutTangent) const
{
	// 폴리라인이 없거나 배열 크기가 어긋나면 실패
	if (mPolyLinePoints.Num() < 2 || mPolyLinePoints.Num() != mPolyLineDistances.Num())
	{
		return false;
	}

	// 진행거리가 속한 선분 검색
	// @details
	// 누적거리 오름차순이므로 이진 탐색 사용
	int32 SegmentEndIndex = Algo::UpperBound(mPolyLineDistances, Distance);
	SegmentEndIndex = FMath::Clamp(SegmentEndIndex, 1, mPolyLinePoints.Num() - 1);
	const int32 SegmentStartIndex = SegmentEndIndex - 1;

	// 선분 시작점과 끝점 사이에서 진행거리가 차지하는 비율 (접선공식)
	const float SegmentLength = mPolyLineDistances[SegmentEndIndex] - mPolyLineDistances[SegmentStartIndex];
	const float Alpha = (SegmentLength > KINDA_SMALL_NUMBER)
		? FMath::Clamp((Distance - mPolyLineDistances[SegmentStartIndex]) / SegmentLength, 0.0f, 1.0f)
		: 1.0f;

	// 위치: 선분 위 보간
	// 접선: 선분의 단위 방향벡터
	OutLocation = FMath::Lerp(mPolyLinePoints[SegmentStartIndex], mPolyLinePoints[SegmentEndIndex], Alpha);
	OutTangent = (SegmentLength > KINDA_SMALL_NUMBER)
		? (mPolyLinePoints[SegmentEndIndex] - mPolyLinePoints[SegmentStartIndex]) / SegmentLength
		: FVector::ZeroVector;
	return true;
}

void AUnit::ResetPolyLineState()
{
	ResetForcedMovePresentation();

	// 폴리라인 관련 상태를 모두 비워서 직선이동모드로 전환
	mPolyLinePoints.Reset();
	mPolyLineDistances.Reset();
	mStepMarkerDistances.Reset();
	mCurrentMoveStep = 0;
	mPolyLineTraveledDistance = 0.0f;
}

void AUnit::BakePolyLinePoints(
	const TArray<FVector>& PathPoints,
	float CornerCutRatio,
	float CornerTension,
	TArray<FVector>& OutPoints,
	TArray<float>& OutDistances,
	TArray<float>& OutStepMarkerDistances)
{
	OutPoints.Reset();
	OutDistances.Reset();
	OutStepMarkerDistances.Reset();

	// 경로가 2칸 미만이면 폴리라인 불성립
	if (PathPoints.Num() < 2)
	{
		return;
	}

	/**
	 * @brief 코너당 베지어곡선 분할 수 (짝수 유지 - 중간 분할점을 도착 판정 지점으로 사용)
     * @details
     * 좀 더 부드러운 애니메이션을 원하면 더 잘게 분할하면 됨
     * 포인트 계산이라 성능 영향은 별로 없을 듯
     * @note
     * 분할 수이지 점의 개수가 아님
     */
	constexpr int32 CornerSampleCount = 8;
	const float CutRatio = FMath::Clamp(CornerCutRatio, 0.0f, 0.5f);
	const float Tension = FMath::Clamp(CornerTension, 0.0f, 2.0f);

	// 점을 추가하고 그 점까지의 누적거리를 반환하는 헬퍼
	auto AppendPoint = [&OutPoints, &OutDistances](const FVector& Point) -> float
	{
	    // 이전 점과의 거리가 사실상 0이면
	    // -> 추가할 의미가 없고
	    // -> 0으로 나누기 하는 위험도 있으므로
	    // 추가하지 않고 넘어감
		if (OutPoints.Num() > 0 && FVector::DistSquared(OutPoints.Last(), Point) < KINDA_SMALL_NUMBER)
		{
			return OutDistances.Last();
		}

		const float Distance = (OutPoints.Num() > 0)
			? OutDistances.Last() + FVector::Dist(OutPoints.Last(), Point)
			: 0.0f;
		OutPoints.Add(Point);
		OutDistances.Add(Distance);
		return Distance;
	};

	// 시작점 (시작 타일의 도착 판정 거리는 0)
	OutStepMarkerDistances.Add(AppendPoint(PathPoints[0]));

	// 경로의 중간 지점들(시작/도착 제외) 처리
	// 직진이면 중점 그대로,
	// 꺾이는 곳이면, 코너 컷(곡선의 시작과 끝) 후 베지어곡선 점들 추가
	for (int32 i = 1; i < PathPoints.Num() - 1; ++i)
	{
		const FVector& PrevPoint = PathPoints[i - 1];    // 직전 타일 중점
		const FVector& CornerPoint = PathPoints[i];      // 이번 타일 중점
		const FVector& NextPoint = PathPoints[i + 1];    // 다음 타일 중점

		// 진입 벡터(직전->이번), 진출 벡터(이번->다음)
		const FVector InDelta = CornerPoint - PrevPoint;
		const FVector OutDelta = NextPoint - CornerPoint;

		// 코너 컷 거리 = 구간 길이 x 컷 비율
		const float CutInDistance = InDelta.Size() * CutRatio;
		const float CutOutDistance = OutDelta.Size() * CutRatio;

		// 1) 직진: 진입/진출 방향이 같으면 커브 없이 중점 그대로 추가
		const bool IsStraight =
		    // 평행하고
		    FVector::Parallel(InDelta.GetSafeNormal(), OutDelta.GetSafeNormal()) &&
		    // 두 벡터의 사이각이 90도 이내(내적이 0보다 큰)면 -> 방향이 같음
			FVector::DotProduct(InDelta, OutDelta) > 0.0f;
		if (IsStraight || CutInDistance < KINDA_SMALL_NUMBER || CutOutDistance < KINDA_SMALL_NUMBER)
		{
			// 중점의 누적거리가 곧 이 타일의 도착 판정 거리
			OutStepMarkerDistances.Add(AppendPoint(CornerPoint));
			continue;
		}

		// 2) 코너: 커브 시작점/끝점을 구함
		// 커브 시작점은 코너 중점에서 코너 컷 비율만큼 먼저 시작
		const FVector CurveStart = CornerPoint - InDelta.GetSafeNormal() * CutInDistance;
	    // 커브 끝점은 코너 중점에서 코너 컷 비율만큼 나중에 끝남
		const FVector CurveEnd = CornerPoint + OutDelta.GetSafeNormal() * CutOutDistance;

		// 베지어곡선의 컨트롤 포인트: 텐션에 따라 시작점과 끝점을 잇는 현의 중점과 코너 중점 사이에 위치
		// 텐션 0 = 현의 중점 = 대각선 직진
		// 텐션 1 = 코너 중점 = 표준 베지어
		// 텐션 2 = 코너 중점 반대편 = 곡선이 코너 중점을 통과 (각진 코너링)
		const FVector ChordMidPoint = (CurveStart + CurveEnd) * 0.5f;
		const FVector ControlPoint = FMath::Lerp(ChordMidPoint, CornerPoint, Tension);

		// 시작점과 끝점 사이를 베지어곡선을 n분할해서 만듦
		float MarkerDistance = 0.0f;
		AppendPoint(CurveStart);
		for (int32 Sample = 1; Sample <= CornerSampleCount; ++Sample)
		{
		    // T는 1/8, 2/8, ... 8/8 식으로 변화
			const float T = static_cast<float>(Sample) / CornerSampleCount;
		    /*
		     * @brief 베지어곡선상의 점 Point는 다음 두 지점을 잇는 선분의 T/8 지점이다.
		     * @details
		     *   기준이 되는 선분은 두 개가 고정이다.
		     *   하나는 CurveStart에서 ControlPoint를 향하는 선분(진입선분)
		     *   또 하나는 ControlPoint에서 CurveEnd를 향하는 선분(진출선분)
		     *
		     *   앞의 선분을 L1, 뒤의 선분을 L2라고 하면,
		     *   첫번째 베지어곡선상의 점 S1은, L1의 1/8 지점과 L2의 1/8 지점을 잇는 선분의 1/8 지점
		     *   두번째 베지어곡선상의 점 S2는, L1의 2/8 지점과 L2의 2/8 지점을 잇는 선분의 2/8 지점
		     *   ...
		     *   마지막 베지어곡선상의 점 S8은, L1의 8/8 지점(ControlPoint)과 L2의 8/8 지점(진출점) 을 잇는 선분의 8/8 지점(진출점)
		     *   이런식으로 분할 개수만큼 반복한다.
		     */
			const FVector Point = FMath::Lerp(
				FMath::Lerp(CurveStart, ControlPoint, T),
				FMath::Lerp(ControlPoint, CurveEnd, T),
				T);
			const float Distance = AppendPoint(Point);

			// 곡선의 중간 점을 이 타일의 도착 판정 거리로 기록
		    // @note 그래서 분할 개수는 짝수로 하는 게 좋음
			if (Sample == CornerSampleCount / 2)
			{
				MarkerDistance = Distance;
			}
		}
		OutStepMarkerDistances.Add(MarkerDistance);
	}

	// 도착점 설정
	// 마지막 타일의 도착 판정 거리이며 폴리라인 전체 길이와 동일함
	OutStepMarkerDistances.Add(AppendPoint(PathPoints.Last()));
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

UArrowComponent* AUnit::GetArrowComponent() const
{
	return mArrowComp;
}
