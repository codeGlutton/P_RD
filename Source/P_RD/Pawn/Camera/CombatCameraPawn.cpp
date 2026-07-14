// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Camera/CombatCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Input/InputData.h"
#include "InputCoreTypes.h"
#include "GameMode/CombatGameMode.h"

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
		}
	}

	UpdateWorldPressInput(PlayerController, DeltaTime);


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


	if (PlayerInputComponent == nullptr)
	{
		return;
	}

	// EnhancedInputComponent는 UE 5.7에서 레거시 BindKey/BindTouch 오버로드를 막는다.
	// 월드 포인터는 별도 InputAction 자산 없이 동작해야 하므로 기본 InputComponent에 바인딩한다.
	// UMG 버튼이 입력을 Handled로 소비하면 여기까지 내려오지 않는다. 즉 월드에서 시작한 포인터만 추적한다.
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ACombatCameraPawn::HandleWorldTouchPressed);
	PlayerInputComponent->BindTouch(IE_Released, this, &ACombatCameraPawn::HandleWorldTouchReleased);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ACombatCameraPawn::HandleWorldMousePressed);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ACombatCameraPawn::HandleWorldMouseReleased);
}

void ACombatCameraPawn::HandleWorldTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex != ETouchIndex::Touch1)
	{
		CancelWorldPress();
		return;
	}

	mWorldPointerSource = EWorldPointerSource::Touch;
	mWorldTouchFingerIndex = FingerIndex;
	mWorldPressGesture.Begin(FVector2D(Location.X, Location.Y));
}

void ACombatCameraPawn::HandleWorldTouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (mWorldPointerSource != EWorldPointerSource::Touch || FingerIndex != mWorldTouchFingerIndex)
	{
		return;
	}

	CompleteWorldPress(FVector2D(Location.X, Location.Y));
}

void ACombatCameraPawn::HandleWorldMousePressed()
{
	if (mWorldPointerSource != EWorldPointerSource::None)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (PlayerController == nullptr || PlayerController->GetMousePosition(MouseX, MouseY) == false)
	{
		return;
	}

	mWorldPointerSource = EWorldPointerSource::Mouse;
	mWorldPressGesture.Begin(FVector2D(MouseX, MouseY));
}

void ACombatCameraPawn::HandleWorldMouseReleased()
{
	if (mWorldPointerSource != EWorldPointerSource::Mouse)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (PlayerController == nullptr || PlayerController->GetMousePosition(MouseX, MouseY) == false)
	{
		CancelWorldPress();
		return;
	}

	CompleteWorldPress(FVector2D(MouseX, MouseY));
}

void ACombatCameraPawn::UpdateWorldPressInput(APlayerController* PlayerController, float DeltaTime)
{
	if (PlayerController == nullptr || mWorldPressGesture.IsActive() == false)
	{
		return;
	}

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	bool bStillPressed = false;

	if (mWorldPointerSource == EWorldPointerSource::Touch)
	{
		const int32 TouchIndex = StaticCast<int32>(mWorldTouchFingerIndex);
		if (mTouchStates.IsValidIndex(TouchIndex) == false || mTouchStates.IsValidIndex(1) == false)
		{
			CancelWorldPress();
			return;
		}

		// 두 번째 손가락이 들어오면 핀치/카메라 제스처이므로 월드 탭을 취소한다.
		if (mTouchStates[1].bIsCurrentlyPressed)
		{
			CancelWorldPress();
			return;
		}

		ScreenPosition = mTouchStates[TouchIndex].CurTouchPos;
		bStillPressed = mTouchStates[TouchIndex].bIsCurrentlyPressed;
	}
	else if (mWorldPointerSource == EWorldPointerSource::Mouse)
	{
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (PlayerController->GetMousePosition(MouseX, MouseY) == false)
		{
			CancelWorldPress();
			return;
		}
		ScreenPosition = FVector2D(MouseX, MouseY);
		bStillPressed = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
	}
	else
	{
		CancelWorldPress();
		return;
	}

	// Slate가 해제 이벤트를 소비한 경우에도 물리 입력 상태로 누락 없이 종료한다.
	if (bStillPressed == false)
	{
		CompleteWorldPress(ScreenPosition);
		return;
	}

	const EWorldPressGestureResult Result = mWorldPressGesture.Update(
		ScreenPosition,
		DeltaTime,
		mWorldLongPressThreshold,
		mWorldPressMoveTolerance);
	if (Result == EWorldPressGestureResult::LongPress)
	{
		SubmitWorldPress(ScreenPosition, true);
	}
	else if (mWorldPressGesture.IsActive() == false)
	{
		mWorldPointerSource = EWorldPointerSource::None;
	}
}

void ACombatCameraPawn::CompleteWorldPress(const FVector2D& ScreenPosition)
{
	const EWorldPressGestureResult Result = mWorldPressGesture.End(ScreenPosition, mWorldPressMoveTolerance);
	mWorldPointerSource = EWorldPointerSource::None;
	if (Result == EWorldPressGestureResult::Tap)
	{
		SubmitWorldPress(ScreenPosition, false);
	}
}

void ACombatCameraPawn::CancelWorldPress()
{
	mWorldPressGesture.Cancel();
	mWorldPointerSource = EWorldPointerSource::None;
}

void ACombatCameraPawn::SubmitWorldPress(const FVector2D& ScreenPosition, bool bLongPress) const
{
	ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr;
	if (CombatGameMode != nullptr)
	{
		CombatGameMode->HandleCombatWorldTouch(ScreenPosition, bLongPress);
	}
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

