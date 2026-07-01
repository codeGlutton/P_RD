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

	FVector Center = mDefaultPos;
	FVector Extent = mMoveClampBox;
	FQuat Rotation = GetOwner()->GetActorQuat(); // 액터의 회전 값을 쿼터니언으로 전달

	DrawDebugBox(
		GetWorld(),
		Center,
		Extent,
		Rotation, // 여기에 회전 값을 넣으면 기울어진 박스가 그려집니다.
		FColor::Green,
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

	FHitResult HitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 1000.0f); // 1000유닛 앞

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

	FVector LocationPos = HitResult.ImpactPoint;
	FVector CameraPos = GetCameraPos(); // 카메라가 바라보고 있는 땅의 위치를 가져온다.
	UE_LOG(LogTemp, Warning, TEXT("LocPos : %f, %f, %f"), LocationPos.X, LocationPos.Y, LocationPos.Z);
	UE_LOG(LogTemp, Warning, TEXT("CameraPos : %f, %f, %f"), CameraPos.X, CameraPos.Y, CameraPos.Z);
	LocationPos.Z = 0;
	CameraPos.Z = 0;


	// 변환된 WorldLocation로 액터의 위치를 변경합니다.
	GetOwner()->AddActorWorldOffset(LocationPos - CameraPos);
}

void UCameraMovementComponent::MoveToLocation(FVector LocationPos)
{

	FVector CameraPos = GetCameraPos();	// 카메라가 바라보고 있는 땅의 위치를 가져온다.
	UE_LOG(LogTemp, Warning, TEXT("LocPos : %f, %f, %f"), LocationPos.X, LocationPos.Y, LocationPos.Z);
	UE_LOG(LogTemp, Warning, TEXT("CameraPos : %f, %f, %f"), CameraPos.X, CameraPos.Y, CameraPos.Z);
	LocationPos.Z = 0;
	CameraPos.Z = 0;


	// 변환된 WorldLocation로 액터의 위치를 변경합니다.
	GetOwner()->AddActorWorldOffset(LocationPos - CameraPos);
}

FVector UCameraMovementComponent::GetCameraPos()
{
	FHitResult HitResult;
	FVector StartTrace = GetOwner()->GetActorLocation(); // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 1000.0f); // 1000유닛 앞

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

	return HitResult.ImpactPoint;
}
