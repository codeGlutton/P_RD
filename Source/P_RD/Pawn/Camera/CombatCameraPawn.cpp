// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Camera/CombatCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Component/TimeScaleComponent/TimeScaleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Input/InputData.h"
#include "InputCoreTypes.h"
#include "EnhancedInputComponent.h"

#if !UE_BUILD_SHIPPING
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Singleton/WorldSubsystem/SRPGCommandRouterModel.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Pawn/UnitModel.h"
#include "SRPGFramework/SRPGMoveAction.h"
#endif

// Sets default values
ACombatCameraPawn::ACombatCameraPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	mSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneComponent"));
	RootComponent = mSceneComponent;

	//mSpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	//mSpringArmComponent->SetRelativeRotation(FRotator(-30, 0, 0));

	{
		mCameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
		mCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
		mCameraComponent->OrthoWidth = 2000.0f;
		mCameraComponent->bAutoCalculateOrthoPlanes = false;
		mCameraComponent->OrthoNearClipPlane = -2000.f;
		mCameraComponent->OrthoFarClipPlane = 20000.f;
		//mCameraComponent->bCameraMeshHiddenInGame = false;
		mCameraComponent->SetupAttachment(mSceneComponent);
	}

	{
		mCameraMovementComponent = CreateDefaultSubobject<UCameraMovementComponent>("CameraMovementComponent");
		mCameraMovementComponent->SetCameraComponent(mCameraComponent);
		//mCameraMovementComponent->SetSpringArmComponent(mSpringArmComponent);
	}

	{
		mTimeScaleComponent = CreateDefaultSubobject<UTimeScaleComponent>("TimeScaleComponent");
	}

	// 카메라 회전은 컨트롤러 회전을 그대로 따라감
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = true;
}

// Called when the game starts or when spawned
void ACombatCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	mTouchStates.SetNum(2);

	OnDragging.AddUObject(this, &ACombatCameraPawn::Dragging);
	OnPinching.AddUObject(this, &ACombatCameraPawn::Pinching);
	
}

// Called every frame
void ACombatCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 카메라는 UMG 이벤트가 아니라 raw touch를 직접 폴링한다. 모달 UI가 잠근
	// 동안에는 이전 프레임 좌표도 지워서, 팝업을 닫은 손이 곧바로 드래그로
	// 이어지지 않게 한다.
	if (mTouchGestureInputEnabled == false)
	{
		for (FTouchState& TouchState : mTouchStates)
		{
			TouchState = FTouchState();
		}
		return;
	}

	AController* DefaultController = GetController();
	// DefaultController가 존재하지 않으면 함수를 종료합니다.
	if (!ensureMsgf(IsValid(DefaultController), TEXT("컨트롤러가 없습니다")))
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	// PlayerController가 존재하지 않으면 함수를 종료합니다.
	if (!ensureMsgf(IsValid(PlayerController), TEXT("Player 컨트롤러가 없습니다")))
	{
		return;
	}

	FTouchState Touches[2];
	for (int32 Index = 0; Index < 2; ++Index)
	{
		PlayerController->GetInputTouchState(static_cast<ETouchIndex::Type>(Index),
			Touches[Index].CurTouchPos.X, Touches[Index].CurTouchPos.Y,
			Touches[Index].bIsCurrentlyPressed);
	}
	FVector2D MousePosition = FVector2D::ZeroVector;
	const bool bMousePressed = PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y)
		&& PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
	UpdatePointerGestures(Touches[0], Touches[1], bMousePressed, MousePosition);
}

void ACombatCameraPawn::UpdatePointerGestures(const FTouchState& FirstTouch,
	const FTouchState& SecondTouch, bool bMousePressed, const FVector2D& MousePosition)
{
	mTouchStates.SetNum(2);
	if (!mTouchGestureInputEnabled)
	{
		for (FTouchState& State : mTouchStates) State = FTouchState();
		return;
	}
	// Real/emulated touch wins, so mouse-for-touch never produces two gestures.
	const bool bUseMouse = !FirstTouch.bIsCurrentlyPressed && !SecondTouch.bIsCurrentlyPressed
		&& bMousePressed;
	if (bUseMouse != mUsingMouseGesture)
	{
		for (FTouchState& State : mTouchStates) State = FTouchState();
	}
	mUsingMouseGesture = bUseMouse;
	FTouchState Samples[2] = { FirstTouch, SecondTouch };
	if (bUseMouse)
	{
		Samples[0].bIsCurrentlyPressed = true;
		Samples[0].CurTouchPos = MousePosition;
	}
	// Mouse and touch share the same camera projection, bounds and modal gate.
	for (int i = 0; i < 2; ++i)
	{
		mTouchStates[i].PreTouchPos = mTouchStates[i].CurTouchPos;
		bool bPreTickTouch = mTouchStates[i].bIsCurrentlyPressed;
		mTouchStates[i].CurTouchPos = Samples[i].CurTouchPos;
		mTouchStates[i].bIsCurrentlyPressed = Samples[i].bIsCurrentlyPressed;

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

UTimeScaleComponent* ACombatCameraPawn::GetTimeScaleComponent()
{
	return mTimeScaleComponent.Get();
}

void ACombatCameraPawn::SetTouchGestureInputEnabled(const bool bEnabled)
{
	if (mTouchGestureInputEnabled == bEnabled)
	{
		return;
	}
	mTouchGestureInputEnabled = bEnabled;
	for (FTouchState& TouchState : mTouchStates)
	{
		TouchState = FTouchState();
	}
}

bool ACombatCameraPawn::IsDrag()
{
	return mTouchStates[0].bIsCurrentlyPressed &&
		!mTouchStates[1].bIsCurrentlyPressed &&
		mImageStabilization < FVector2D::Distance(mTouchStates[0].StartTouchPos, mTouchStates[0].CurTouchPos) &&
		!mTouchStates[0].PreTouchPos.Equals(mTouchStates[0].CurTouchPos, 0.01f);
}

bool ACombatCameraPawn::IsPinch()
{
	float PrePinchDis = FVector2D::Distance(mTouchStates[0].PreTouchPos, mTouchStates[1].PreTouchPos);
	float CurPinchDis = FVector2D::Distance(mTouchStates[0].CurTouchPos, mTouchStates[1].CurTouchPos);

	return mTouchStates[0].bIsCurrentlyPressed &&
		mTouchStates[1].bIsCurrentlyPressed &&
		mImageStabilization < FMath::Abs(PrePinchDis - CurPinchDis);
}

void ACombatCameraPawn::Dragging(const TArray<FTouchState>& Touch1State)
{
	if (!ensureMsgf(IsValid(mCameraMovementComponent), TEXT("CameraMovementComponent가 없습니다")))
	{
		return;
	}

	mCameraMovementComponent.Get()->DragMoveToViewportPosition_Instant(Touch1State[0].PreTouchPos, Touch1State[0].CurTouchPos);
}

void ACombatCameraPawn::Pinching(const TArray<FTouchState>& TouchState)
{
	if (!ensureMsgf(IsValid(mCameraMovementComponent), TEXT("CameraMovementComponent가 없습니다")))
	{
		return;
	}

	float PrePinchDis = FVector2D::Distance(mTouchStates[0].PreTouchPos, mTouchStates[1].PreTouchPos);
	float CurPinchDis = FVector2D::Distance(mTouchStates[0].CurTouchPos, mTouchStates[1].CurTouchPos);

	//mCameraMovementComponent.Get()->ZoomCamera_Instant(PrePinchDis - CurPinchDis);
	mCameraMovementComponent.Get()->PinchZoomCamera_InstantAndMoveToViewportPosition_Instant(PrePinchDis - CurPinchDis, (mTouchStates[0].CurTouchPos + mTouchStates[1].CurTouchPos)/2);
}

