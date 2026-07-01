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
	// 확대, 축소 최소 최대 거리
	mCameraComponent->OrthoWidth = FMath::Clamp(mCameraComponent->OrthoWidth, mMinOrthoWidth, mMaxOrthoWidth);
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

void UCameraMovementComponent::ZoomCamera(float ZoomValue)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// OW 값을 추가합니다.
	float OW = mCameraComponent->OrthoWidth + ZoomValue * mZoomSpeed;
	mCameraComponent->OrthoWidth = FMath::Clamp(OW, mMinOrthoWidth, mMaxOrthoWidth);
}

void UCameraMovementComponent::ZoomCameraAndMoveToViewport(float ZoomValue, FVector2D ViewPortPos)
{
	ZoomCamera(ZoomValue);
	MoveToViewport(ViewPortPos);
}

void UCameraMovementComponent::MoveToViewport(FVector2D ViewPortPos)
{
	FVector WorldLocation;
	FVector WorldDirection;

	// ViewPort를 WorldLocation으로 변환
	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X,ViewPortPos.Y, WorldLocation, WorldDirection);

	// 변환된 WorldLocation로 액터의 위치를 변경합니다.
	GetOwner()->SetActorLocation(WorldLocation);
}
