#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Pawn/Camera/OrthographicCameraShakePattern.h"
#include "FunctionLibrary/CameraFunctionLibrary.h"

UCameraMovementComponent::UCameraMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
}

void UCameraMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	InitializeCameraGround();
	InitializeCameraTargetLocation();
}

void UCameraMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCameraMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 강조가 완전히 끝났는지 판단하는 로직입니다.
	// Emphasis_Returning 중이고 현재 위치가 타겟 위치로 완전히 돌아왔다면 기본 상태로 되돌립니다.
	if (mCamerControlState == ECameraControlState::Emphasis_Returning)
	{
		// 도착 판정: 여기서만 임계값을 쓰되, 목적은 "재진입 허용 시점" 판단이지
		// "저장할 위치가 정확한지" 판단이 아니므로 훨씬 안전함
		if (FVector2D::Distance(mCurrentLookAtCameraLocation, mTargetLookAtCameraLocation) < 1.f && abs(mCurZoom - mTargetZoom) < 1.f)
		{
			mCamerControlState = ECameraControlState::Normal;
		}
	}

	// 카메라의 위치를 초기화합니다.
	// 한번만 실행 되어야 합니다.
	if (!mInitCameraLocation)
	{
		InitializeCameraTargetLocation();
	}

	/*
	* @note 매 틱 중복 RayCast 문제
	* 거의 매틱마다 RayCast를 여러함수에서 중복하여 쏘기 때문에 비용이 적지 않다고 생각됩니다.
	* 추후 시간적 여유가 생긴다면 해당 RayCast를 SegmentPlaneIntersection로 변경하는 것을 고려해보는 것이 좋다고 생각됩니다.
	*/

	MoveSmooth(DeltaTime);		// 카메라를 이동시키는 함수입니다.
	ZoomSmooth(DeltaTime);
	ShakeZoom();
	FollowActor();				// 강조 시 액터가 있다면 액터를 따라가는 함수입니다.
	ClampingCamera();			// 카메라의 Location, OrthoWidth를 제한하는 함수입니다.
}

#pragma region GetSet

void UCameraMovementComponent::SetCameraComponent(UCameraComponent* CameraComponent)
{
	mCameraComponent = CameraComponent;
}

UCameraComponent* UCameraMovementComponent::GetCameraComponent()
{
	return mCameraComponent.Get();
}

//void UCameraMovementComponent::SetZoomSpeed(float ZoomSpeed)
//{
//	mZoomSpeed = ZoomSpeed;
//}

void UCameraMovementComponent::SetMaxOrthoWidth(float MaxOrthoWidth)
{
	mMaxOrthoWidth = MaxOrthoWidth;
}

void UCameraMovementComponent::SetMinOrthoWidth(float MinOrthoWidth)
{
	mMinOrthoWidth = MinOrthoWidth;
}

void UCameraMovementComponent::SetMoveClampingBoxCenter(FVector MoveClampingBoxCenter)
{
	mMoveClampingBoxCenter = MoveClampingBoxCenter;
}

void UCameraMovementComponent::SetMoveClampingBox(FVector2D MoveClampingBox)
{
	mMoveClampingBox = MoveClampingBox;
}

void UCameraMovementComponent::SetEmphasisZoom(float EmphasisZoom)
{
	mEmphasisZoom = EmphasisZoom;
}

void UCameraMovementComponent::SetViewportOffset(FVector2D ViewportOffset)
{
	mViewportOffset = ViewportOffset;
}

FVector2D UCameraMovementComponent::GetViewportOffset() const
{
	return mViewportOffset;
}

#pragma endregion

#pragma region Initialize

void UCameraMovementComponent::InitializeCameraGround()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor != nullptr)
	{
		const FVector PlaneLocation = OwnerActor->GetActorLocation() + FVector(0, 0, -100.f);
		const FVector PlaneNormal = FVector::UpVector;

		mGroundPlane = FPlane(PlaneLocation, PlaneNormal);
	}
}

void UCameraMovementComponent::InitializeCameraTargetLocation()
{
	FVector CameraCenterRayCastImpactPoint;
	FVector CameraOffsetRayCastImpactPoint;

	bool bCameraHit = GetCameraRayImpactPoint(CameraCenterRayCastImpactPoint);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	bCameraHit = GetCameraRayImpactPointWithOffset(CameraOffsetRayCastImpactPoint);

	// 카메라의 오프셋에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector2D CurLocation = FVector2D(CameraOffsetRayCastImpactPoint);

	mTargetLookAtCameraLocation = CurLocation;
	mInitCameraLocation = true;

	mMoveClampingBoxCenter = CameraCenterRayCastImpactPoint;

	mCurZoom = mCameraComponent->OrthoWidth;
	mTargetZoom = mCurZoom;
}

#pragma endregion

#pragma region Helper

bool UCameraMovementComponent::GetCameraGroundImpactPoint(const FVector& StartTrace, const FVector& EndTrace, OUT FVector& ImpactPoint)
{
	bool bHit = FMath::SegmentPlaneIntersection(StartTrace, EndTrace, mGroundPlane, OUT ImpactPoint);

	return bHit;
}

bool UCameraMovementComponent::GetCameraRayImpactPoint(OUT FVector& ImpactPoint)
{
	FVector StartTrace = GetOwner()->GetActorLocation(); // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	return GetCameraGroundImpactPoint(StartTrace, EndTrace, OUT ImpactPoint);
}

bool UCameraMovementComponent::GetCameraRayImpactPointWithOffset(OUT FVector& ImpactPoint)
{
	const FVector2D ViewportPos = UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(1., 1.) - mViewportOffset);
	return GetScreenRayImpactPoint(ViewportPos, OUT ImpactPoint);
}

/**
 * @brief 화면 픽셀 좌표에서 카메라 판(CameraMove 채널)으로 Ray 를 쏜다.
 */
bool UCameraMovementComponent::GetScreenRayImpactPoint(FVector2D ScreenPosition, OUT FVector& ImpactPoint)
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController == nullptr)
	{
		return false;
	}

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (PlayerController->DeprojectScreenPositionToWorld(
		ScreenPosition.X, ScreenPosition.Y,
		OUT WorldLocation, OUT WorldDirection) == false)
	{
		return false;
	}

	return GetCameraGroundImpactPoint(WorldLocation, WorldLocation + GetOwner()->GetActorForwardVector() * 100000.0f, ImpactPoint);
}

FVector UCameraMovementComponent::ClampLocationWithinRotatedBox(const FVector& WorldLocation)
{
	FRotator OwnerRotator = FRotator(0, GetOwner()->GetActorRotation().Yaw + mMoveClampingBoxRotation,0);

	// 1. 월드 -> 박스 로컬 공간으로 변환 (중심 기준으로 이동 후, 역회전)
	FVector LocalLocation = OwnerRotator.UnrotateVector(WorldLocation - mMoveClampingBoxCenter);

	// 2. 로컬 공간에서는 축 정렬이므로 그냥 Clamp
	LocalLocation.X = FMath::Clamp(LocalLocation.X, -mMoveClampingBox.X / 2, mMoveClampingBox.X / 2);
	LocalLocation.Y = FMath::Clamp(LocalLocation.Y, -mMoveClampingBox.Y / 2, mMoveClampingBox.Y / 2);
	// 필요하면 Z도
	// LocalLocation.Z = FMath::Clamp(LocalLocation.Z, -BoxExtentFull.Z / 2, BoxExtentFull.Z / 2);

	// 3. 다시 박스 회전 적용 후 중심 좌표 더해서 월드로 복귀
	FVector ClampedWorldLocation = OwnerRotator.RotateVector(LocalLocation) + mMoveClampingBoxCenter;

	return ClampedWorldLocation;
}

#pragma endregion

#pragma region Update

void UCameraMovementComponent::ClampingCamera()
{
	if (!ensureMsgf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다")))
	{
		return;
	}

	// =============================
	// 확대, 축소 최소 최대 Clamping
	mCurZoom = FMath::Clamp(mCurZoom, mMinOrthoWidth, mMaxOrthoWidth);

	//=================================
	// 카메라 이동의 제한을 둡니다.
	// 카메라의 크기는 무시힌다.

	// 카메라의 시선 위치를 구합니다.
	FVector CameraCenterRayCastImpactPoint;
	bool bCameraHit = GetCameraRayImpactPointWithOffset(CameraCenterRayCastImpactPoint);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// 클램핑 시 영역을 구합니다.
	FVector ActorLocation = ClampLocationWithinRotatedBox(CameraCenterRayCastImpactPoint);

	mCurrentLookAtCameraLocation += FVector2D(ActorLocation - CameraCenterRayCastImpactPoint);

	// 위치의 차를 이용하여 카메라의 위치를 옮깁니다.
	GetOwner()->AddActorWorldOffset(ActorLocation - CameraCenterRayCastImpactPoint);

	mTargetLookAtCameraLocation = FVector2D(ClampLocationWithinRotatedBox(FVector(mTargetLookAtCameraLocation,0.f)));

	// 편집기 플레이에서도 실제 전투 화면을 평가할 수 있도록 이동 제한 박스와
	// 카메라 방향 디버그 선을 렌더하지 않는다. 제한 계산 자체는 위에서 유지된다.

		//==============================
	// 이동 제한 범위를 보여줍니다.
	// @note 
	// 직교 카메라로 보고 있을 때는 제한 범위가 이상해 보일 것입니다.
	// 에디터 카메라로 보아야지 제한 범위가 납득이 되실 것입니다.
	FVector Center = mMoveClampingBoxCenter;
	FVector Extent = FVector(mMoveClampingBox / 2, 0);
	FQuat Rotation = FRotator(0.f, GetOwner()->GetActorRotation().Yaw + mMoveClampingBoxRotation, 0.f).Quaternion();

	/*
#if WITH_EDITOR

	DrawDebugBox(
		GetWorld(),
		Center,
		Extent,
		Rotation,
		FColor::Red,
		false,
		-1.0f,
		0,
		5.0f
	);

	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + (GetOwner()->GetActorForwardVector() * 100000.0f), FColor::Green);

#endif
	*/
}

void UCameraMovementComponent::MoveSmooth(float DeltaTime)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 카메라의 시선 위치를 구합니다.
	FVector CameraCenterRayCastImpactPoint;
	bool bCameraHit = GetCameraRayImpactPoint(CameraCenterRayCastImpactPoint);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	mCurrentLookAtCameraLocation = FVector2D(CameraCenterRayCastImpactPoint);

	FVector NextLocation = FMath::VInterpTo(FVector(mCurrentLookAtCameraLocation, 0.f), FVector(mTargetLookAtCameraLocation, 0.f), DeltaTime, mMoveSpeed);
	FVector CurLocation = FVector(mCurrentLookAtCameraLocation, 0.f);
	GetOwner()->AddActorWorldOffset(NextLocation - CurLocation);
}

void UCameraMovementComponent::ZoomSmooth(float DeltaTime)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 선형 보간 적용
	float NewZoom = FMath::FInterpTo(mCurZoom, mTargetZoom, DeltaTime, mZoomSpeed);

	mCameraComponent->OrthoWidth = FMath::Clamp(NewZoom, mMinOrthoWidth, mMaxOrthoWidth);
	mCurZoom = mCameraComponent->OrthoWidth;
}

void UCameraMovementComponent::ShakeZoom()
{
	// 끝난 셰이크는 참조를 비워줌
	if (IsValid(mActiveCameraShakeInstance) && mActiveCameraShakeInstance->IsFinished())
	{
		mActiveCameraShakeInstance = nullptr;
	}

	// 현재 ShakeInstance에서 OrthoWidthValue를 가져옵니다.
	float OrthoOffset = 0.f;
	if (IsValid(mActiveCameraShakeInstance))
	{
		if (UOrthographicCameraShakePattern* Pattern =
			Cast<UOrthographicCameraShakePattern>(mActiveCameraShakeInstance->GetRootShakePattern()))
		{
			OrthoOffset = Pattern->GetCurrentOrhoWidthValue();
		}
	}

	// 현재 Zoom에서 OrthoOffset을 변경하여 흔듭니다.
	mCameraComponent->OrthoWidth = mCurZoom + OrthoOffset;
}

void UCameraMovementComponent::FollowActor()
{
	if (!ensureMsgf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다")))
	{
		return;
	}

	if (mCamerControlState != ECameraControlState::Emphasis)
	{
		return;
	}

	if (!mEmphasisActor.IsValid())
	{
		return;
	}

	FVector WorldPosition = mEmphasisActor->GetActorLocation();

	FVector2D ViewportPosition;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewportPosition);
	ViewportPosition += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	MoveToViewportPosition_Smooth(ViewportPosition);
}

#pragma endregion

#pragma region CameraControl

void UCameraMovementComponent::PinchZoomCamera_InstantAndMoveToViewportPosition_Instant(float ZoomDelta, FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 확대 전 ViewPortPos 구하기
	// =========================================
	// ViewPortPos를 World 좌표로 변환합니다.
	FVector WorldLocation;
	FVector WorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	FVector ViewPortImpactPoint;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	bool bViewPortHit = GetCameraGroundImpactPoint(StartTrace, EndTrace, ViewPortImpactPoint);

	// 충돌되지 않았다면 취소
	if (!ensureMsgf(bViewPortHit, TEXT("ViewPortPos에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector PrePos = ViewPortImpactPoint;
	PrePos.Z = 0;

	// ===============================================================

	// 카메라를 줌 한다.
	ZoomCamera_Instant(ZoomDelta);

	// 확대 후 ViewPortPos 구하기
	// =========================================
	// ViewPortPos를 World 좌표로 변환합니다.
	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	StartTrace = WorldLocation; // 시작 지점
	EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	bViewPortHit = GetCameraGroundImpactPoint(StartTrace, EndTrace, ViewPortImpactPoint);

	if (!ensureMsgf(bViewPortHit, TEXT("ViewPortPos에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector NextPos = ViewPortImpactPoint;
	NextPos.Z = 0;

	// ============================================================

	// 위치의 차를 이용하여 카메라의 위치를 옮깁니다.
	MoveToDeltaPosition_Instant(PrePos - NextPos);
}

void UCameraMovementComponent::DragMoveToViewportPosition_Instant(FVector2D PreViewPortPos, FVector2D CurViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector PreWorldLocation;
	FVector PreWorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(PreViewPortPos.X, PreViewPortPos.Y, PreWorldLocation, PreWorldDirection);

	FVector ViewPortImpactPoint;
	FVector StartTrace = PreWorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	bool bViewPortHit = GetCameraGroundImpactPoint(StartTrace, EndTrace, ViewPortImpactPoint);

	if (!ensureMsgf(bViewPortHit, TEXT("Viewport에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector PrePos = ViewPortImpactPoint;
	PrePos.Z = 0;

	//======================================
	FVector CurWorldLocation;
	FVector CurWorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(CurViewPortPos.X, CurViewPortPos.Y, CurWorldLocation, CurWorldDirection);

	//ViewPortHitResult;
	StartTrace = CurWorldLocation; // 시작 지점
	EndTrace = StartTrace + (CurWorldDirection * 100000.0f);

	// 채널 설정 (ECC_Visibility 등)
	//QueryParams;
	//QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bViewPortHit = GetCameraGroundImpactPoint(StartTrace, EndTrace, ViewPortImpactPoint);

	if (!ensureMsgf(bViewPortHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector CurPos = ViewPortImpactPoint;
	CurPos.Z = 0;

	// 목표 위치와 현재 카메라 위치를 같이 변경합니다.
	mTargetLookAtCameraLocation += FVector2D(PrePos - CurPos);
	GetOwner()->AddActorWorldOffset(PrePos - CurPos);
}
#pragma endregion

#pragma region SimpleMove
void UCameraMovementComponent::MoveToWorldPosition(FVector WorldPosition, bool IsInstantMove)
{
	FVector2D ViewportPosition;
	auto* Controller = GetWorld()->GetFirstPlayerController();
	Controller->ProjectWorldLocationToScreen(WorldPosition, ViewportPosition);
	ViewportPosition += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	if (IsInstantMove == true)
	{
		MoveToViewportPosition_Instant(ViewportPosition);
	}
	else
	{
		MoveToViewportPosition_Smooth(ViewportPosition);
	}
}

void UCameraMovementComponent::StartFollowingActor(AActor* FollowingActor)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	mEmphasisActor = FollowingActor;

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::EndFollowingActor()
{
	// 카메라의 강조 상태를 종료합니다.
	mCamerControlState = ECameraControlState::Emphasis_Returning;

	mEmphasisActor = nullptr;
}
#pragma endregion

#pragma region Emphasis
void UCameraMovementComponent::StartEmphasisToWorldPositionWithZoom(float TargetZoom, FVector WorldPosition)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector CameraRayImpactPoint;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayImpactPoint(CameraRayImpactPoint))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	FVector2D ViewportPosition;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewportPosition);
	ViewportPosition += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(TargetZoom, ViewportPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPositionWithZoom(float TargetZoom, FVector2D ViewPortPos)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector CameraRayImpactPoint;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayImpactPoint(CameraRayImpactPoint))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	ViewPortPos += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(TargetZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;

}

void UCameraMovementComponent::StartEmphasisToActorWithZoom(float TargetZoom, AActor* EmphasisActor)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector CameraRayImpactPoint;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayImpactPoint(CameraRayImpactPoint))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	// 액터의 현재 위치를 구합니다.
	FVector WorldPosition = EmphasisActor->GetActorLocation();

	FVector2D ViewportPosition;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewportPosition);
	ViewportPosition += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(TargetZoom, ViewportPosition);

	mEmphasisActor = EmphasisActor;

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToWorldPosition(FVector WorldPosition)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector CameraRayImpactPoint;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayImpactPoint(CameraRayImpactPoint))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	FVector2D ViewportPosition;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewportPosition);
	ViewportPosition += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(mEmphasisZoom, ViewportPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPosition(FVector2D ViewPortPos)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector CameraRayImpactPoint;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayImpactPoint(CameraRayImpactPoint))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	ViewPortPos += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(mEmphasisZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToActor(AActor* EmphasisActor)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector CameraRayImpactPoint;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayImpactPoint(CameraRayImpactPoint))
	{
		// 아직 강조 상태라면 이전값을 유지
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	// 액터의 현재 위치를 구합니다.
	FVector WorldPosition = EmphasisActor->GetActorLocation();

	FVector2D ViewportPosition;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewportPosition);
	ViewportPosition += UCameraFunctionLibrary::GetSizeOnMainViewport(this, FVector2D(0.5, 0.5) - mViewportOffset);

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(mEmphasisZoom, ViewportPosition);

	mEmphasisActor = EmphasisActor;

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::EndEmphasis()
{
	// 카메라의 강조 상태를 종료합니다.
	mCamerControlState = ECameraControlState::Emphasis_Returning;

	mEmphasisActor = nullptr;

	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mPreDefaultState.Zoom, mPreDefaultState.Position);
}
#pragma endregion

#pragma region CameraShake


void UCameraMovementComponent::StartCameraShake(TSubclassOf<class UCameraShakeBase> CameraShakeClass)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	APlayerController* FirstPlayerController = GetWorld()->GetFirstPlayerController();
	checkf(FirstPlayerController != nullptr, TEXT("PlayerController가 없습니다"));

	if (!ensureMsgf(CameraShakeClass != nullptr, TEXT("Camera Shake Class가 존재하지 않습니다.")))
	{
		return;
	}

	if (UCameraFunctionLibrary::IsCameraShakePossible(this) == false)
	{
		return;
	}

	mActiveCameraShakeInstance = FirstPlayerController->PlayerCameraManager->StartCameraShake(CameraShakeClass);
}

#pragma endregion

#pragma region Zoom

void UCameraMovementComponent::ZoomCamera_Instant(float ZoomDelta)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));
	mCurZoom = FMath::Clamp(mCurZoom + ZoomDelta, mMinOrthoWidth, mMaxOrthoWidth);
	mCameraComponent->OrthoWidth = mCurZoom;
	mTargetZoom = mCurZoom;
}

void UCameraMovementComponent::ZoomCamera_Smooth(float TargetZoom)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	mTargetZoom = TargetZoom;
}

void UCameraMovementComponent::ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(float TargetZoom, FVector2D ViewPortPos)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	ZoomCamera_Smooth(TargetZoom);
	MoveToViewportPosition_Smooth(ViewPortPos);
}

void UCameraMovementComponent::ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(float TargetZoom, FVector WorldPosition)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	ZoomCamera_Smooth(TargetZoom);
	MoveToWorldPosition_Smooth(WorldPosition);
}
#pragma endregion

#pragma region Move

void UCameraMovementComponent::MoveToViewportPosition_Instant(FVector2D ViewPortPos)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	//==============================
	// ViewPortPos를 WorldLocation으로 변환하고 Lay를 쏩니다.
	FVector WorldLocation;
	FVector WorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	FVector ViewPortImpactPoint;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	bool bViewPortHit = GetCameraGroundImpactPoint(StartTrace, EndTrace, ViewPortImpactPoint);

	if (!ensureMsgf(bViewPortHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ============================================
	// 카메라의 시선으로로 Ray를 쏜다.
	FVector CameraCenterRayImpactPoint;
	bool bCameraHit = GetCameraRayImpactPoint(CameraCenterRayImpactPoint);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ==========================================
	// 카메라의 위치를 이동시킨다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector LocationPos = ViewPortImpactPoint;
	FVector CameraPos = CameraCenterRayImpactPoint; // 카메라가 바라보고 있는 땅의 위치를 가져온다.
	LocationPos.Z = 0;
	CameraPos.Z = 0;

	mTargetLookAtCameraLocation = FVector2D(LocationPos);
	GetOwner()->AddActorWorldOffset(LocationPos - CameraPos);
}

void UCameraMovementComponent::MoveToWorldPosition_Instant(FVector WorldPosition)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// LocationPos를 카메라에 투영하고 다시 Raycast를 계산한다.
	FVector2D ViewPort;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewPort);

	MoveToViewportPosition_Instant(ViewPort);
}

void UCameraMovementComponent::MoveToViewportPosition_Smooth(FVector2D ViewPortPos)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	//==============================
	// ViewPortPos를 WorldLocation으로 변환하고 Lay를 쏩니다.
	FVector WorldLocation;
	FVector WorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	FVector ViewPortImpactPoint;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	bool bViewPortHit = GetCameraGroundImpactPoint(StartTrace, EndTrace, ViewPortImpactPoint);

	if (!ensureMsgf(bViewPortHit, TEXT("ViewPort에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ==========================================
	// 카메라의 최종 위치를 변경합니다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector LocationPos = ViewPortImpactPoint;
	LocationPos.Z = 0;

	mTargetLookAtCameraLocation = FVector2D(LocationPos);
}

void UCameraMovementComponent::MoveToWorldPosition_Smooth(FVector WorldPosition)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// WorldPosition를 카메라에 투영하고 다시 Raycast를 계산한다.
	FVector2D ViewPort;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewPort);

	MoveToViewportPosition_Smooth(ViewPort);
}

void UCameraMovementComponent::MoveToDeltaPosition_Instant(FVector DeltaPosition)
{
	mCurrentLookAtCameraLocation += FVector2D(DeltaPosition);
	mTargetLookAtCameraLocation = mCurrentLookAtCameraLocation;

	// 위치의 차를 이용하여 카메라의 위치를 옮깁니다.
	GetOwner()->AddActorWorldOffset(DeltaPosition);
}


#pragma endregion


