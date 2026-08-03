// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Pawn/Camera/CombatCameraPlane.h"
#include "GameFramework/SpringArmComponent.h"
#include "Pawn/Camera/OrthographicCameraShakePattern.h"

// Sets default values for this component's properties
UCameraMovementComponent::UCameraMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCameraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

	// 초기 시작 위치를 중심으로 설정합니다.
	//mMoveClampingBoxCenter = GetOwner()->GetActorLocation();
	//mMoveClampingBoxCenter.Z = 0.f;

	// ACombatCameraPlane을 생성합니다.
	// ACombatCameraPlane은 카메라 조작을 위해서 반드시 필요한 액터입니다
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 카메라보다 아래에 카메라판을 생성합니다.
	FVector SpawnPos = GetOwner()->GetActorLocation();
	SpawnPos.Z -= 100;

	mCameraPlane = GetWorld()->SpawnActor<ACombatCameraPlane>(
		ACombatCameraPlane::StaticClass(),
		SpawnPos,
		FRotator::ZeroRotator,
		SpawnParams
	);
}

void UCameraMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 카메라가 제거 될 때 판도 같이 제거합니다.
	if (mCameraPlane.IsValid())
	{
		mCameraPlane->Destroy();
		mCameraPlane = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

// Called every frame
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

//void UCameraMovementComponent::SetSpringArmComponent(USpringArmComponent* SpringComponent)
//{
//	mSpringArmComponent = SpringComponent;
//}

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

#pragma endregion

#pragma region ETC

void UCameraMovementComponent::InitializeCameraTargetLocation()
{
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector2D CurLocation = FVector2D(CameraCenterRayCastHitResult.ImpactPoint);

	mTargetLookAtCameraLocation = CurLocation;
	mInitCameraLocation = true;

	mMoveClampingBoxCenter = CameraCenterRayCastHitResult.ImpactPoint;

	mCurZoom = mCameraComponent->OrthoWidth;
	mTargetZoom = mCurZoom;
}


bool UCameraMovementComponent::GetCameraRayHitPoint(OUT FHitResult& HitResult)
{
	FVector StartTrace = GetOwner()->GetActorLocation(); // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bHit = GetWorld()->LineTraceSingleByObjectType(
		HitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	return bHit;
}

FVector UCameraMovementComponent::ClampLocationWithinRotatedBox(const FVector& WorldLocation)
{
	FRotator OwnerRotator = FRotator(0, GetOwner()->GetActorRotation().Yaw,0);

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
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// 클램핑 시 영역을 구합니다.
	FVector ActorLocation = ClampLocationWithinRotatedBox(CameraCenterRayCastHitResult.ImpactPoint);

	mCurrentLookAtCameraLocation += FVector2D(ActorLocation - CameraCenterRayCastHitResult.ImpactPoint);

	// 위치의 차를 이용하여 카메라의 위치를 옮깁니다.
	GetOwner()->AddActorWorldOffset(ActorLocation - CameraCenterRayCastHitResult.ImpactPoint);

	mTargetLookAtCameraLocation = FVector2D(ClampLocationWithinRotatedBox(FVector(mTargetLookAtCameraLocation,0.f)));

	// 편집기 플레이에서도 실제 전투 화면을 평가할 수 있도록 이동 제한 박스와
	// 카메라 방향 디버그 선을 렌더하지 않는다. 제한 계산 자체는 위에서 유지된다.
}

void UCameraMovementComponent::MoveSmooth(float DeltaTime)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 카메라의 시선 위치를 구합니다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	mCurrentLookAtCameraLocation = FVector2D(CameraCenterRayCastHitResult.ImpactPoint);

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

	FHitResult ViewPortHitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams,
		QueryParams
	);

	// 충돌되지 않았다면 취소
	if (!ensureMsgf(bViewPortHit, TEXT("ViewPortPos에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector PrePos = ViewPortHitResult.ImpactPoint;
	PrePos.Z = 0;

	// ===============================================================

	// 카메라를 줌 한다.
	ZoomCamera_Instant(ZoomDelta);

	// 카메라를 갱신하여 ProjMat을 갱신해줍니다.
	GetWorld()->GetFirstPlayerController()->PlayerCameraManager->UpdateCamera(0.0f);

	// 확대 후 ViewPortPos 구하기
	// =========================================
	// ViewPortPos를 World 좌표로 변환합니다.
	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	StartTrace = WorldLocation; // 시작 지점
	EndTrace = StartTrace + (WorldDirection * 100000.0f);

	bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams,
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("ViewPortPos에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector NextPos = ViewPortHitResult.ImpactPoint;
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

	FHitResult ViewPortHitResult;
	FVector StartTrace = PreWorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (PreWorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("Viewport에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector PrePos = ViewPortHitResult.ImpactPoint;
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

	bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector CurPos = ViewPortHitResult.ImpactPoint;
	CurPos.Z = 0;

	// 목표 위치와 현재 카메라 위치를 같이 변경합니다.
	mTargetLookAtCameraLocation += FVector2D(PrePos - CurPos);
	GetOwner()->AddActorWorldOffset(PrePos - CurPos);
}

#pragma endregion

#pragma region Emphasis
void UCameraMovementComponent::StartEmphasisToWorldPositionWithZoom(float TargetZoom, FVector WorldPosition)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(TargetZoom, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPositionWithZoom(float TargetZoom, FVector2D ViewPortPos)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(TargetZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;

}

void UCameraMovementComponent::StartEmphasisToActorWithZoom(float TargetZoom, AActor* EmphasisActor)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	// 액터의 현재 위치를 구합니다.
	FVector WorldPosition = EmphasisActor->GetActorLocation();

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(TargetZoom, WorldPosition);

	mEmphasisActor = EmphasisActor;

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToWorldPosition(FVector WorldPosition)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mEmphasisZoom, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPosition(FVector2D ViewPortPos)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(mEmphasisZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToActor(AActor* EmphasisActor)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 강조 상태라면 이전값을 유지
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCurZoom;
	}

	// 액터의 현재 위치를 구합니다.
	FVector WorldPosition = EmphasisActor->GetActorLocation();

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mEmphasisZoom, WorldPosition);

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

	FHitResult ViewPortHitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ============================================
	// 카메라의 시선으로로 Ray를 쏜다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ==========================================
	// 카메라의 위치를 이동시킨다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector LocationPos = ViewPortHitResult.ImpactPoint;
	FVector CameraPos = CameraCenterRayCastHitResult.ImpactPoint; // 카메라가 바라보고 있는 땅의 위치를 가져온다.
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

	FHitResult ViewPortHitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("ViewPort에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ============================================
	// 카메라의 시선으로로 Ray를 쏜다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ==========================================
	// 카메라의 최종 위치를 변경합니다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector LocationPos = ViewPortHitResult.ImpactPoint;
	FVector CameraPos = CameraCenterRayCastHitResult.ImpactPoint; // 카메라가 바라보고 있는 땅의 위치를 가져온다.
	LocationPos.Z = 0;
	CameraPos.Z = 0;

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

#pragma region Emphasis

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

	MoveToWorldPosition_Smooth(WorldPosition);
}


#pragma endregion
