// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Camera/CombatCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Input/InputData.h"

// Sets default values
ACombatCameraPawn::ACombatCameraPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	mSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneComponent"));
	mSceneComponent->SetRelativeRotation(FRotator(-30, 0, 0));
	RootComponent = mSceneComponent;

	//mSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	//mSpringArmComponent->SetRelativeRotation(FRotator(-30, 0, 0));

	mCameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	mCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	mCameraComponent->OrthoWidth = 1024.0f;
	//mCameraComponent->bCameraMeshHiddenInGame = false;
	mCameraComponent->SetupAttachment(mSceneComponent);

	mCameraMovementComponent = CreateDefaultSubobject<UCameraMovementComponent>("CameraMovementComponent");
	mCameraMovementComponent->SetCameraComponent(mCameraComponent);
	//mCameraMovementComponent->SetSpringArmComponent(mSpringArmComponent);


}

// Called when the game starts or when spawned
void ACombatCameraPawn::BeginPlay()
{
	Super::BeginPlay();


	// PlayerController를 얻어온다.
	TObjectPtr<APlayerController>	PlayerController = GetController<APlayerController>();


	mTouchStates.SetNum(2);

	OnDragging.AddUObject(this, &ACombatCameraPawn::Dragging);
	OnPinching.AddUObject(this, &ACombatCameraPawn::Pinching);
	
}

// Called every frame
void ACombatCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	APlayerController* PC = Cast<APlayerController>(GetController());
	checkf(PC, TEXT("PC가 없습니다."));

	// 터치 상태를 보고 제스처를 판단한다.
	for (int i = 0; i < 2; ++i)
	{
		mTouchStates[i].PreTouchPos = mTouchStates[i].CurTouchPos;
		bool bPreTickTouch = mTouchStates[i].bIsCurrentlyPressed;
		PC->GetInputTouchState((ETouchIndex::Type)i, mTouchStates[i].CurTouchPos.X, mTouchStates[i].CurTouchPos.Y, mTouchStates[i].bIsCurrentlyPressed);

		if (bPreTickTouch == 0 && mTouchStates[i].bIsCurrentlyPressed)
		{
			mTouchStates[i].StartTouchPos = mTouchStates[i].CurTouchPos;
			mTouchStates[i].PreTouchPos = mTouchStates[i].CurTouchPos;

		}
	}


	// Pinch 중
	if (IsPinch())
	{
		if (OnPinching.IsBound())
		{
			OnPinching.Broadcast(mTouchStates);
		}
	}
	// 드래그 중
	else if (IsDrag())
	{
		if (OnDragging.IsBound())
		{
			OnDragging.Broadcast(mTouchStates);
		}
	}
}

// Called to bind functionality to input
void ACombatCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	// 인자로 들어온 InputComponent를 EnhancedInputComponent로 형변환한다.
	// 언리얼 오브젝트는 항상 Cast<Type>() 함수를 이용해서 형변환한다.
	TObjectPtr<UEnhancedInputComponent>	Input =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);
}

UCameraComponent* ACombatCameraPawn::GetCameraComponent()
{
	return mCameraComponent.Get();
}

UCameraMovementComponent* ACombatCameraPawn::GetCameraMovementComponent()
{
	return mCameraMovementComponent.Get();
}

bool ACombatCameraPawn::IsDrag()
{
	return mTouchStates[0].bIsCurrentlyPressed &&
		3.f < FVector2D::Distance(mTouchStates[0].PreTouchPos, mTouchStates[0].CurTouchPos);
}

bool ACombatCameraPawn::IsPinch()
{
	float PrePinchDis = FVector2D::Distance(mTouchStates[0].PreTouchPos, mTouchStates[1].PreTouchPos);
	float CurPinchDis = FVector2D::Distance(mTouchStates[0].CurTouchPos, mTouchStates[1].CurTouchPos);

	return mTouchStates[0].bIsCurrentlyPressed &&
		mTouchStates[1].bIsCurrentlyPressed &&
		3.f < FMath::Abs(PrePinchDis - CurPinchDis);
}

void ACombatCameraPawn::Dragging(const TArray<FTouchState>& Touch1State)
{
	checkf(IsValid(mCameraMovementComponent), TEXT("CameraMovementComponent Is Not Valid"));

	mCameraMovementComponent.Get()->DragMoveToViewportPosition(Touch1State[0].PreTouchPos, Touch1State[0].CurTouchPos);
}

void ACombatCameraPawn::Pinching(const TArray<FTouchState>& TouchState)
{
	checkf(IsValid(mCameraMovementComponent), TEXT("CameraMovementComponent Is Not Valid"));

	float PrePinchDis = FVector2D::Distance(mTouchStates[0].PreTouchPos, mTouchStates[1].PreTouchPos);
	float CurPinchDis = FVector2D::Distance(mTouchStates[0].CurTouchPos, mTouchStates[1].CurTouchPos);

	mCameraMovementComponent.Get()->ZoomCamera_Instant(PrePinchDis - CurPinchDis);
}