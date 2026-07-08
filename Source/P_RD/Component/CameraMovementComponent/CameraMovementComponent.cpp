// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

// @note checkf(Camera) 할 것

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
	mMoveClampingBoxCenter = GetOwner()->GetActorLocation();
	mMoveClampingBoxCenter.Z = 0.f;

}


// Called every frame
void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// ...
	// =============================
	// 확대, 축소 최소 최대 거리
	mCameraComponent->OrthoWidth = FMath::Clamp(mCameraComponent->OrthoWidth, mMinOrthoWidth, mMaxOrthoWidth);

	//=================================
	// 카메라 이동의 제한을 둡니다.
	// 카메라의 크기는 무시힌다.
	FVector ActorLocation = GetOwner()->GetActorLocation();

	ActorLocation.X = FMath::Clamp(ActorLocation.X, mMoveClampingBoxCenter.X - mMoveClampingBox.X / 2, mMoveClampingBoxCenter.X + mMoveClampingBox.X / 2);
	ActorLocation.Y = FMath::Clamp(ActorLocation.Y, mMoveClampingBoxCenter.Y - mMoveClampingBox.Y / 2, mMoveClampingBoxCenter.Y + mMoveClampingBox.Y / 2);

	GetOwner()->SetActorLocation(ActorLocation);

	//==============================
	// 이동 제한 범위를 보여줍니다.
	// @note 수학 공식 이상함. 하지만 현재 각도에서는 우선 원하는 제한 범위가 나오므로 후 순위
	// FVector Center = mMoveClampingBoxCenter + FMath::Sin(FMath::Abs(FMath::DegreesToRadians(GetOwner()->GetActorRotation().Pitch))) * GetOwner()->GetActorLocation().Z;
	FVector Center = mMoveClampingBoxCenter;
	FVector Extent = FVector(mMoveClampingBox /2, 0);
	FQuat Rotation = FQuat();

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


}

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

void UCameraMovementComponent::ZoomCamera_Instant(float ZoomDelta)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));
	mCameraComponent->OrthoWidth = FMath::Clamp(mCameraComponent->OrthoWidth + ZoomDelta, mMinOrthoWidth, mMaxOrthoWidth);
}

void UCameraMovementComponent::ZoomCamera_InstantAndMoveToViewportPosition_Instant(float ZoomDelta, FVector2D ViewPortPos)
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

	// 채널 설정 (ECC_Visibility 등)
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByChannel(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ECC_Visibility, // 충돌 채널
		QueryParams
	);

	if (!bViewPortHit)
		return;

	FVector PrePos = ViewPortHitResult.ImpactPoint;
	PrePos.Z = 0;

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

	bViewPortHit = GetWorld()->LineTraceSingleByChannel(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ECC_Visibility, // 충돌 채널
		QueryParams
	);

	if (!bViewPortHit)
		return;

	FVector NextPos = ViewPortHitResult.ImpactPoint;
	NextPos.Z = 0;
	
	// 위치의 차를 이용하여 카메라의 위치를 옮깁니다.
	GetOwner()->AddActorWorldOffset(PrePos - NextPos);
}

void UCameraMovementComponent::ZoomCamera_Smooth(float TargetZoom)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 0.01초 간격으로 호출할 예정이므로 (Duration / 0.01)번 반복
	mStartZoom = mCameraComponent->OrthoWidth;
	mCurZoom = mStartZoom;
	mEndZoom = TargetZoom;

	// 0.01초 간격으로 호출할 예정이므로 (Duration / 0.01)번 반복
	mCurrentZoomAlpha = 0.0f;

	// Timer로 Zoom을 수행합니다.
	GetWorld()->GetTimerManager().SetTimer(mTimerHandle_Zoom, this, &UCameraMovementComponent::ZoomSmooth, 0.01f, true);
}

void UCameraMovementComponent::ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(float ZoomDelta, FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	ZoomCamera_Smooth(ZoomDelta);
	MoveToViewportPosition_Smooth(ViewPortPos);
}

void UCameraMovementComponent::ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(float ZoomDelta, FVector WorldPosition)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	ZoomCamera_Smooth(ZoomDelta);
	MoveToWorldPosition_Smooth(WorldPosition);
}

void UCameraMovementComponent::MoveToViewportPosition_Smooth(FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	//==============================
	// ViewPortPos를 WorldLocation으로 변환하고 Lay를 쏩니다.
	FVector WorldLocation;
	FVector WorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X,ViewPortPos.Y, WorldLocation, WorldDirection);

	FHitResult ViewPortHitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 100000.0f);

	// 채널 설정 (ECC_Visibility 등)
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByChannel(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ECC_Visibility, // 충돌 채널
		QueryParams
	);

	if (!bViewPortHit)
		return;

	// ============================================
	// 카메라의 시선으로로 Ray를 쏜다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!bCameraHit)
		return;

	// ==========================================
	// 카메라의 위치를 이동시킨다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector LocationPos = ViewPortHitResult.ImpactPoint;
	FVector CameraPos = CameraCenterRayCastHitResult.ImpactPoint; // 카메라가 바라보고 있는 땅의 위치를 가져온다.
	LocationPos.Z = 0;
	CameraPos.Z = 0;

	// 0.01초 간격으로 호출할 예정이므로 (Duration / 0.01)번 반복
	mStartLocation = CameraPos;
	mCurLocation = CameraPos;
	mEndLocation = LocationPos;

	// 0.01초 간격으로 호출할 예정이므로 (Duration / 0.01)번 반복
	mCurrentMoveAlpha = 0.0f;

	// Timer로 이동을 수행합니다.
	GetWorld()->GetTimerManager().SetTimer(mTimerHandle_Move, this, &UCameraMovementComponent::MoveSmooth, 0.01f, true);
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

	// 채널 설정 (ECC_Visibility 등)
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByChannel(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ECC_Visibility, // 충돌 채널
		QueryParams
	);

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
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bViewPortHit = GetWorld()->LineTraceSingleByChannel(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ECC_Visibility, // 충돌 채널
		QueryParams
	);

	FVector CurPos = ViewPortHitResult.ImpactPoint;
	CurPos.Z = 0;

	GetOwner()->AddActorWorldOffset(PrePos - CurPos);
}

void UCameraMovementComponent::MoveToWorldPosition_Smooth(FVector LocationPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// =====================================
	// 카메라의 중심을 기준으로 레이를 쏜다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!bCameraHit)
		return;

	// ==========================================
	// 카메라의 위치를 이동시킨다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector CameraPos = CameraCenterRayCastHitResult.ImpactPoint;
	LocationPos.Z = 0;
	CameraPos.Z = 0;

	mStartLocation = CameraPos;
	mCurLocation = CameraPos;
	mEndLocation = LocationPos;

	// 0.01초 간격으로 호출할 예정이므로 (Duration / 0.01)번 반복
	mCurrentMoveAlpha = 0.0f;

	// Timer로 이동을 수행합니다.
	GetWorld()->GetTimerManager().SetTimer(mTimerHandle_Move, this, &UCameraMovementComponent::MoveSmooth, 0.01f, true);

}

void UCameraMovementComponent::StartEmphasisToWorldPositionWithZoomDelta(float TargetZoom, FVector WorldPosition)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(TargetZoom, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;

}

void UCameraMovementComponent::StartEmphasisToViewPortPositionWithZoomDelta(float TargetZoom, FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(TargetZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;

}

void UCameraMovementComponent::StartEmphasisToWorldPosition(FVector WorldPosition)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mEmphasisZoom, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPosition(FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = GetWorld()->GetTimerManager().IsTimerActive(mTimerHandle_Move) ? 
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(mEmphasisZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::EndEmphasis()
{
	// 카메라의 강조 상태를 종료합니다.
	mCamerControlState = ECameraControlState::Normal;

	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mPreDefaultState.Zoom, mPreDefaultState.Position);
}

bool UCameraMovementComponent::GetCameraRayHitPoint(OUT FHitResult& HitResult)
{
	FVector StartTrace = GetOwner()->GetActorLocation(); // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	// 채널 설정 (ECC_Visibility 등)
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartTrace,
		EndTrace,
		ECC_Visibility, // 충돌 채널
		QueryParams
	);

	return bHit;
}

void UCameraMovementComponent::MoveSmooth()
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// mMoveDuration 시간동안 이동 합니다.
	mCurrentMoveAlpha += 0.01f / mMoveDuration; // 시간에 따른 진행도 증가

	if (mCurrentMoveAlpha >= 1.0f)
	{
		mCurrentMoveAlpha = 1.0f;
		GetWorld()->GetTimerManager().ClearTimer(mTimerHandle_Move); // 타이머 종료
	}

	// 선형 보간 적용
	FVector NewLocation = FMath::InterpEaseInOut(mStartLocation, mEndLocation, mCurrentMoveAlpha, mMoveExp);

	GetOwner()->AddActorWorldOffset(NewLocation - mCurLocation);

	mCurLocation = NewLocation;

	//=================================
	// 카메라 이동의 제한을 둡니다.
	// 카메라의 크기는 무시힌다.
	FVector ActorLocation = GetOwner()->GetActorLocation();

	ActorLocation.X = FMath::Clamp(ActorLocation.X, mMoveClampingBoxCenter.X - mMoveClampingBox.X / 2, mMoveClampingBoxCenter.X + mMoveClampingBox.X / 2);
	ActorLocation.Y = FMath::Clamp(ActorLocation.Y, mMoveClampingBoxCenter.Y - mMoveClampingBox.Y / 2, mMoveClampingBoxCenter.Y + mMoveClampingBox.Y / 2);

	GetOwner()->SetActorLocation(ActorLocation);
}

void UCameraMovementComponent::ZoomSmooth()
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// mZoomDuration 시간동안 줌을 합니다.
	mCurrentZoomAlpha += 0.01f / mZoomDuration; // 시간에 따른 진행도 증가

	if (mCurrentZoomAlpha >= 1.0f)
	{
		mCurrentZoomAlpha = 1.0f;
		GetWorld()->GetTimerManager().ClearTimer(mTimerHandle_Zoom); // 타이머 종료
	}

	// 선형 보간 적용
	float NewZoom= FMath::InterpEaseInOut(mStartZoom, mEndZoom, mCurrentZoomAlpha, mZoomExp);

	mCameraComponent->OrthoWidth = FMath::Clamp(NewZoom, mMinOrthoWidth, mMaxOrthoWidth);

}
