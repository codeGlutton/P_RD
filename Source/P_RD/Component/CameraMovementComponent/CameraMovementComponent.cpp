// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Camera/CameraComponent.h"

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

	// ...
}

void UCameraMovementComponent::SetCameraComponent(UCameraComponent* CameraComponent)
{
	mCameraComponent = CameraComponent;
}

UCameraComponent* UCameraMovementComponent::GetCameraComponent()
{
	return mCameraComponent.Get();
}

void UCameraMovementComponent::SetZoomSpeed(float ZoomSpeed)
{
	mZoomSpeed = ZoomSpeed;
}

void UCameraMovementComponent::Zoom(float ZoomValue, FVector2D ScreenCenter)
{
	// A. 줌 전 중심점 월드 좌표 저장
	//FVector WorldPos_Before = DeprojectToWorld(ScreenCenter);

	float OW = mCameraComponent->OrthoWidth + ZoomValue * mZoomSpeed;
	mCameraComponent->OrthoWidth = FMath::Clamp(OW, mMinOrthoWidth, mMaxOrthoWidth);

	// C. 줌 후 중심점 월드 좌표 확인 및 보정
	//FVector WorldPos_After = DeprojectToWorld(ScreenCenter);

	// D. 차이만큼 카메라 위치 이동
	//GetOwner()->AddActorWorldOffset(WorldPos_Before - WorldPos_After);
}
