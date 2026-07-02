// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

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

void UCameraMovementComponent::SetZoomSpeed(float ZoomSpeed)
{
	mZoomSpeed = ZoomSpeed;
}

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

void UCameraMovementComponent::ZoomCamera(float ZoomDelta)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// OW 값을 추가합니다.
	float OW = mCameraComponent->OrthoWidth + ZoomDelta * mZoomSpeed;
	mCameraComponent->OrthoWidth = FMath::Clamp(OW, mMinOrthoWidth, mMaxOrthoWidth);
}

void UCameraMovementComponent::ZoomCameraAndMoveToViewportPosition(float ZoomDelta, FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	ZoomCamera(ZoomDelta);
	MoveToViewportPosition(ViewPortPos);
}

void UCameraMovementComponent::ZoomCameraAndMoveToWorldPosition(float ZoomDelta, FVector WorldPosition)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	ZoomCamera(ZoomDelta);
	MoveToWorldPosition(WorldPosition);
}

void UCameraMovementComponent::MoveToViewportPosition(FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

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

	// 변환된 WorldLocation로 액터의 위치를 변경합니다.
	GetOwner()->AddActorWorldOffset(LocationPos - CameraPos);

	//=================================
	// 카메라 이동에 제한을 둡니다.
	// 카메라의 크기는 무시힌다.
	FVector ActorLocation = GetOwner()->GetActorLocation();

	ActorLocation.X = FMath::Clamp(ActorLocation.X, mMoveClampingBoxCenter.X - mMoveClampingBox.X / 2, mMoveClampingBoxCenter.X + mMoveClampingBox.X / 2);
	ActorLocation.Y = FMath::Clamp(ActorLocation.Y, mMoveClampingBoxCenter.Y - mMoveClampingBox.Y / 2, mMoveClampingBoxCenter.Y + mMoveClampingBox.Y / 2);

	GetOwner()->SetActorLocation(ActorLocation);
}

void UCameraMovementComponent::MoveToWorldPosition(FVector LocationPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

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

	GetOwner()->AddActorWorldOffset(LocationPos - CameraPos);

	//=================================
	// 카메라 이동의 제한을 둡니다.
	// 카메라의 크기는 무시힌다.
	FVector ActorLocation = GetOwner()->GetActorLocation();

	ActorLocation.X = FMath::Clamp(ActorLocation.X, mMoveClampingBoxCenter.X - mMoveClampingBox.X / 2, mMoveClampingBoxCenter.X + mMoveClampingBox.X / 2);
	ActorLocation.Y = FMath::Clamp(ActorLocation.Y, mMoveClampingBoxCenter.Y - mMoveClampingBox.Y / 2, mMoveClampingBoxCenter.Y + mMoveClampingBox.Y / 2);

	GetOwner()->SetActorLocation(ActorLocation);

}

void UCameraMovementComponent::StartEmphasisToWorldPositionWithZoomDelta(float ZoomDelta, FVector WorldPosition)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		mPreDefaultState.Position = CameraRayHitResult.ImpactPoint;
		mPreDefaultState.ZoomDelta = ZoomDelta;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCameraAndMoveToWorldPosition(ZoomDelta, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;

}

void UCameraMovementComponent::StartEmphasisToViewPortPositionWithZoomDelta(float ZoomDelta, FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		mPreDefaultState.Position = CameraRayHitResult.ImpactPoint;
		mPreDefaultState.ZoomDelta = ZoomDelta;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCameraAndMoveToViewportPosition(ZoomDelta, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;

}

void UCameraMovementComponent::StartEmphasisToWorldPosition(FVector WorldPosition)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		mPreDefaultState.Position = CameraRayHitResult.ImpactPoint;
		mPreDefaultState.ZoomDelta = mEmphasisZoom - mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCameraAndMoveToWorldPosition(mPreDefaultState.ZoomDelta, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPosition(FVector2D ViewPortPos)
{
	if (mCamerControlState != ECameraControlState::Normal)
		return;

	// 카메라의 현재 상태를 저장합니다.
	FHitResult CameraRayHitResult;
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		mPreDefaultState.Position = CameraRayHitResult.ImpactPoint;
		mPreDefaultState.ZoomDelta = mEmphasisZoom - mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCameraAndMoveToViewportPosition(mPreDefaultState.ZoomDelta, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::EndEmphasis()
{
	mCamerControlState = ECameraControlState::Normal;

	ZoomCameraAndMoveToWorldPosition(-mPreDefaultState.ZoomDelta, mPreDefaultState.Position);
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