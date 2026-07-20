// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Camera/CombatCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Component/CameraMovementComponent/CameraMovementComponent.h"
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
	// 전투 시야는 고정한다. 터치 드래그/핀치가 유닛 조작과 겹치며 멀미를 유발하므로
	// 카메라 폰은 입력 제스처를 매 프레임 해석하지 않는다.
	PrimaryActorTick.bCanEverTick = false;

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

