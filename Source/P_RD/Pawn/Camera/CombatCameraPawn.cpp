// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Camera/CombatCameraPawn.h"

#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "GameMode/CombatGameMode.h"
#include "UI/Combat/CombatUIModel.h"
#include "Camera/CameraComponent.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Component/TimeScaleComponent/TimeScaleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Input/InputData.h"

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

	mCameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	mCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	mCameraComponent->OrthoWidth = 2000.0f;
	//mCameraComponent->bCameraMeshHiddenInGame = false;
	mCameraComponent->SetupAttachment(mSceneComponent);

	mCameraMovementComponent = CreateDefaultSubobject<UCameraMovementComponent>("CameraMovementComponent");
	mCameraMovementComponent->SetCameraComponent(mCameraComponent);
	//mCameraMovementComponent->SetSpringArmComponent(mSpringArmComponent);

	mTimeScaleComponent = CreateDefaultSubobject<UTimeScaleComponent>("TimeScaleComponent");

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

	// 터치 상태를 보고 제스처를 판단한다.
	for (int i = 0; i < 2; ++i)
	{
		mTouchStates[i].PreTouchPos = mTouchStates[i].CurTouchPos;
		bool bPreTickTouch = mTouchStates[i].bIsCurrentlyPressed;
		PlayerController->GetInputTouchState((ETouchIndex::Type)i, mTouchStates[i].CurTouchPos.X, mTouchStates[i].CurTouchPos.Y, mTouchStates[i].bIsCurrentlyPressed);

		if (bPreTickTouch == 0 && mTouchStates[i].bIsCurrentlyPressed)
		{
			mTouchStates[i].StartTouchPos = mTouchStates[i].CurTouchPos;
			mTouchStates[i].PreTouchPos = mTouchStates[i].CurTouchPos;
			if (i == 0)
			{
				mWasDragged = false;
			}
		}

		// 첫 손가락만 본다. 둘째는 확대할 때만 쓰고 탭으로 치지 않는다.
		if (i == 0)
		{
			if (mTouchStates[0].bIsCurrentlyPressed == true
				&& mTapSlack < FVector2D::Distance(mTouchStates[0].StartTouchPos,
					mTouchStates[0].CurTouchPos))
			{
				mWasDragged = true;
			}
			NotifyTapIfNotDragged(i, bPreTickTouch);
		}
	}


	ApplyWheelZoom(PlayerController);

	// Pinch 중
	if (IsPinch())
	{
		ReleaseEmphasis();
		if (OnPinching.IsBound())
		{
			OnPinching.Broadcast(mTouchStates);
		}
	}
	// 드래그 중
	else if (IsDrag())
	{
		ReleaseEmphasis();
		if (OnDragging.IsBound())
		{
			OnDragging.Broadcast(mTouchStates);
		}
	}
}

/**
 * @brief 손으로 카메라를 움직이면 강조를 푼다.
 *
 * @details
 * 강조 중에는 FollowActor() 가 매 틱 강조 대상 쪽으로 카메라를 되돌린다.
 * 그래서 끌어도 손을 떼면 도로 튕겨 온다 -- 실제로 그렇게 보였다. 사람이
 * 직접 움직이기 시작하면 자동 추적은 그만두는 것이 맞다.
 */
void ACombatCameraPawn::ReleaseEmphasis()
{
	if (IsValid(mCameraMovementComponent) == false)
	{
		return;
	}
	mCameraMovementComponent->EndEmphasis();
}

/**
 * @brief 마우스 휠로 확대/축소한다.
 *
 * PC 에는 손가락이 둘 없다. 핀치와 같은 길로 보내되 화면 가운데를 기준으로
 * 삼는다 -- 휠은 커서 자리를 안 알려 준다.
 * @param PlayerController 휠 값을 읽을 컨트롤러
 */
void ACombatCameraPawn::ApplyWheelZoom(APlayerController* PlayerController)
{
	if (PlayerController == nullptr || IsValid(mCameraMovementComponent) == false)
	{
		return;
	}

	const float Wheel = PlayerController->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (FMath::IsNearlyZero(Wheel))
	{
		return;
	}

	ReleaseEmphasis();
	mCameraMovementComponent->ZoomCamera_Instant(-Wheel * mWheelZoomStep);
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

/**
 * @brief 손을 뗐고 끌지 않았으면 월드 탭을 알린다.
 * @param TouchIndex  몇 번째 손가락인가
 * @param bWasPressed 직전 틱에 눌려 있었나
 */
void ACombatCameraPawn::NotifyTapIfNotDragged(int32 TouchIndex, bool bWasPressed)
{
	const bool bReleased = bWasPressed == true
		&& mTouchStates[TouchIndex].bIsCurrentlyPressed == false;
	if (bReleased == false || mWasDragged == true)
	{
		return;
	}

	ACombatGameMode* CombatGameMode = GetWorld() != nullptr
		? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr;
	if (CombatGameMode == nullptr)
	{
		return;
	}
	if (UCombatUIModel* UIModel = CombatGameMode->GetCombatUIModel())
	{
		// 뗀 자리가 아니라 댄 자리로 보낸다. 뗄 때 손가락이 한두 픽셀 미끄러진
		// 것까지 반영하면 가장자리 타일에서 옆 칸이 잡힌다.
		UIModel->RequestWorldTouch(mTouchStates[TouchIndex].StartTouchPos, false);
	}
}

bool ACombatCameraPawn::IsDrag()
{
	return mTouchStates[0].bIsCurrentlyPressed &&
		mImageStabilization < FVector2D::Distance(mTouchStates[0].PreTouchPos, mTouchStates[0].CurTouchPos);
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
	mCameraMovementComponent.Get()->ZoomCamera_InstantAndMoveToViewportPosition_Instant(PrePinchDis - CurPinchDis, (mTouchStates[0].CurTouchPos + mTouchStates[1].CurTouchPos)/2);
}

void ACombatCameraPawn::RDMoveTo(int32 X, int32 Y)
{
	// 치트는 출시 빌드에서 본문만 비움 (UFUNCTION 선언은 전처리 제외 불가)
#if !UE_BUILD_SHIPPING
	// 전투 모델·타일맵·플레이어 유닛 확보 (전투 중이 아니면 무시)
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	UUnitModel* PlayerUnit = CombatModel != nullptr ? CombatModel->GetPlayerUnit() : nullptr;
	if (TileMap == nullptr || PlayerUnit == nullptr)
	{
		UE_LOG(LogSRPGCombat, Warning, TEXT("RDMoveTo: 전투 진행 중이 아니라 무시"));
		return;
	}

	// 현재 칸 → 목표 칸 최단 경로 계산 (막히거나 맵 밖이면 빈 경로)
	const TArray<FTileIndex> Path = TileMap->FindPath(PlayerUnit->GetTileTransform().mIndex, FTileIndex(X, Y));
	if (Path.Num() < 2)
	{
		UE_LOG(LogSRPGCombat, Warning, TEXT("RDMoveTo: (%d,%d)까지 경로 없음"), X, Y);
		return;
	}

	// 확정 경로를 실은 이동 커맨드 발행 — 빌드 액션 없이 MoveAction 직행 (BuildMove와 동일 형식)
	USRPGCommandRouterModel* CommandRouter = GetWorldSubsystemModel<USRPGCommandRouterModel>(this);
	checkf(CommandRouter != nullptr, TEXT("명령 라우터 모델 nullptr"));

	TInstancedStruct<FSRPGCommand> MoveCommand;
	MoveCommand.InitializeAs<FSRPGMoveCommand>();
	MoveCommand.GetMutable<FSRPGMoveCommand>().mPathTileIndexes = Path;
	const bool Submitted = CommandRouter->SummitCommand(MoveCommand);

	UE_LOG(LogSRPGCombat, Log, TEXT("RDMoveTo: (%d,%d) 경로 %d칸, 커맨드 %s"), X, Y, Path.Num(), Submitted ? TEXT("제출됨") : TEXT("거부됨"));
#endif
}

void ACombatCameraPawn::RDRotate(int32 Direction)
{
	// 치트는 출시 빌드에서 본문만 비움 (UFUNCTION 선언은 전처리 제외 불가)
#if !UE_BUILD_SHIPPING
	// 전투 모델·타일맵·플레이어 유닛 확보 (전투 중이 아니면 무시)
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	UTileMapModel* TileMap = CombatModel != nullptr ? CombatModel->GetTileMap() : nullptr;
	UUnitModel* PlayerUnit = CombatModel != nullptr ? CombatModel->GetPlayerUnit() : nullptr;
	if (TileMap == nullptr || PlayerUnit == nullptr)
	{
		UE_LOG(LogSRPGCombat, Warning, TEXT("RDRotate: 전투 진행 중이 아니라 무시"));
		return;
	}

	// 방향 인덱스 검증 (0=Forward 1=Right 2=Backward 3=Left)
	if (Direction < 0 || Direction >= static_cast<int32>(ETileActorDirection::Count))
	{
		UE_LOG(LogSRPGCombat, Warning, TEXT("RDRotate: 잘못된 방향 %d (0~3)"), Direction);
		return;
	}

	// 배리어 없이 즉발 요청 — 뷰는 내부 무통지 배리어로 회전을 완주
	TileMap->RotateActor(static_cast<ETileActorDirection>(Direction), PlayerUnit);
	UE_LOG(LogSRPGCombat, Log, TEXT("RDRotate: 방향 %d로 전환 요청"), Direction);
#endif
}

