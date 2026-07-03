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
	mCameraComponent->bCameraMeshHiddenInGame = false;
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

	// IsValid : 유효성 검사를 해주는 함수이다. 언리얼 객체가 유효한지를 판단해준다.
	if (IsValid(PlayerController))
	{
		// Enhanced Input System을 얻어온다.
		TObjectPtr<UEnhancedInputLocalPlayerSubsystem>	Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

		// UDefaultInputData CDO를 얻어온다.
		const UCombatInputData* InputData = GetDefault<UCombatInputData>();

		// MappingContext를 등록한다.
		Subsystem->AddMappingContext(InputData->mContext, 0);
	}
	
}

// Called every frame
void ACombatCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ACombatCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);


	// 인자로 들어온 InputComponent를 EnhancedInputComponent로 형변환한다.
	// 언리얼 오브젝트는 항상 Cast<Type>() 함수를 이용해서 형변환한다.
	TObjectPtr<UEnhancedInputComponent>	Input =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);
	
	if (IsValid(Input))
	{
		// UDefaultInputData CDO를 얻어온다.
		const UCombatInputData* InputData = GetDefault<UCombatInputData>();

		// 이동키를 누를때 동작할 함수를 바인딩한다.

		Input->BindAction(InputData->FindAction(TEXT("TouchMove")), ETriggerEvent::Completed,
			this, &ACombatCameraPawn::TouchMoveKey);

		Input->BindAction(InputData->FindAction(TEXT("TouchMove")), ETriggerEvent::Triggered,
			this, &ACombatCameraPawn::TouchTragMoveKey);

		Input->BindAction(InputData->FindAction(TEXT("Zoom")), ETriggerEvent::Triggered,
			this, &ACombatCameraPawn::ZoomKey);

		Input->BindAction(InputData->FindAction(TEXT("Zoom")), ETriggerEvent::Completed,
			this, &ACombatCameraPawn::ZoomEndKey);

		// ===========================================
		// 줌 기능 테스트 
		Input->BindAction(InputData->FindAction(TEXT("FirstTouch")), ETriggerEvent::Started,
			this, &ACombatCameraPawn::FirstTouchStart);

		Input->BindAction(InputData->FindAction(TEXT("FirstTouch")), ETriggerEvent::Completed,
			this, &ACombatCameraPawn::FirstTouchCompleted);

		Input->BindAction(InputData->FindAction(TEXT("SecondTouch")), ETriggerEvent::Triggered,
			this, &ACombatCameraPawn::SecondTouchStart);

		Input->BindAction(InputData->FindAction(TEXT("SecondTouch")), ETriggerEvent::Completed,
			this, &ACombatCameraPawn::SecondTouchCompleted);
	
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


void ACombatCameraPawn::TouchMoveKey(const FInputActionValue& Value)
{
	checkf(IsValid(mCameraMovementComponent), TEXT("CameraMovementComponent Is Not Valid"));

	//// 현재 터치 상태를 가져와서 카메라를 옮깁니다.
	//float X, Y;
	//bool bTouchState;
	//GetWorld()->GetPlayerControllerIterator()->Get()->GetInputTouchState(ETouchIndex::Touch1, X, Y, bTouchState);
	//
	//mCameraMovementComponent->MoveToViewportPosition(FVector2D(X, Y));

	mPreTouchState1.bIsCurrentlyPressed = false;

}

void ACombatCameraPawn::ZoomKey(const FInputActionValue& Value)
{
	checkf(IsValid(mCameraMovementComponent), TEXT("CameraMovementComponent Is Not Valid"));

	// 현재 터치 상태를 가져옵니다.
	FTouchState CurTouchState1;
	FTouchState CurTouchState2;
	GetWorld()->GetPlayerControllerIterator()->Get()->GetInputTouchState(ETouchIndex::Touch1, CurTouchState1.LocationX, CurTouchState1.LocationY, CurTouchState1.bIsCurrentlyPressed);
	GetWorld()->GetPlayerControllerIterator()->Get()->GetInputTouchState(ETouchIndex::Touch2, CurTouchState2.LocationX, CurTouchState2.LocationY, CurTouchState2.bIsCurrentlyPressed);

	// 이전 틱의 터치 상태, 현재 틱의 터치 상태 모두 true라면 Zoom을 합니다.
	if (CurTouchState1.bIsCurrentlyPressed && CurTouchState2.bIsCurrentlyPressed && mPreTouchState1.bIsCurrentlyPressed && mPreTouchState2.bIsCurrentlyPressed)
	{
		FVector2D PreTouch1 = FVector2D(mPreTouchState1.LocationX, mPreTouchState1.LocationY);
		FVector2D PreTouch2 = FVector2D(mPreTouchState2.LocationX, mPreTouchState2.LocationY);
		FVector2D CurTouch1 = FVector2D(CurTouchState1.LocationX, CurTouchState1.LocationY);
		FVector2D CurTouch2 = FVector2D(CurTouchState2.LocationX, CurTouchState2.LocationY);

		float PreDis = FVector2D::Distance(PreTouch1, PreTouch2);
		float CurDis = FVector2D::Distance(CurTouch1, CurTouch2);

		mCameraMovementComponent.Get()->ZoomCamera_Instant(PreDis - CurDis);
	}

	// 이전 Touch 상태를 현재 Touch 상태로 되돌립니다.
	mPreTouchState1 = CurTouchState1;
	mPreTouchState2 = CurTouchState2;
}

void ACombatCameraPawn::ZoomEndKey(const FInputActionValue& Value)
{
	mPreTouchState1.bIsCurrentlyPressed = false;
	mPreTouchState2.bIsCurrentlyPressed = false;
}

void ACombatCameraPawn::TouchTragMoveKey(const FInputActionValue& Value)
{

	// 현재 터치 상태를 가져옵니다.
	FTouchState CurTouchState1;
	GetWorld()->GetPlayerControllerIterator()->Get()->GetInputTouchState(ETouchIndex::Touch1, CurTouchState1.LocationX, CurTouchState1.LocationY, CurTouchState1.bIsCurrentlyPressed);

	// 이전 틱의 터치 상태, 현재 틱의 터치 상태 모두 true라면 Zoom을 합니다.
	if (CurTouchState1.bIsCurrentlyPressed && mPreTouchState1.bIsCurrentlyPressed)
	{
		FVector2D PreTouch1 = FVector2D(mPreTouchState1.LocationX, mPreTouchState1.LocationY);
		FVector2D CurTouch1 = FVector2D(CurTouchState1.LocationX, CurTouchState1.LocationY);

		float PreDis = FVector2D::Distance(PreTouch1, CurTouch1);

		mCameraMovementComponent.Get()->DragMoveToViewportPosition(PreTouch1, CurTouch1);
	}

	// 이전 Touch 상태를 현재 Touch 상태로 되돌립니다.
	mPreTouchState1 = CurTouchState1;
}


// =====================================
// PC에서 Zoom 조작 테스트
void ACombatCameraPawn::FirstTouchStart(const FInputActionValue& Value)
{
	mFirstTouch = true;
	GetWorld()->GetPlayerControllerIterator()->Get()->GetMousePosition(mPreFirstTouch.X, mPreFirstTouch.Y);
}

void ACombatCameraPawn::FirstTouchCompleted(const FInputActionValue& Value)
{
	mFirstTouch = false;
}

void ACombatCameraPawn::SecondTouchStart(const FInputActionValue& Value)
{

	FVector2D CurTouch;
 	GetWorld()->GetPlayerControllerIterator()->Get()->GetMousePosition(CurTouch.X, CurTouch.Y);

	checkf(IsValid(mCameraMovementComponent), TEXT("CameraMovementComponent Is Not Valid"));

	if (mFirstTouch && mSecondTouch)
	{
		FVector2D PreTouch1 = FVector2D(mPreFirstTouch.X, mPreFirstTouch.Y);
		FVector2D PreTouch2 = FVector2D(mPreSecondTouch.X, mPreSecondTouch.Y);
		FVector2D CurTouch1 = FVector2D(mPreFirstTouch.X, mPreFirstTouch.Y);
		FVector2D CurTouch2 = FVector2D(CurTouch.X, CurTouch.Y);

		float PreDis = FVector2D::Distance(PreTouch1, PreTouch2);
		float CurDis = FVector2D::Distance(CurTouch1, CurTouch2);

		mCameraMovementComponent.Get()->ZoomCamera_Instant(PreDis - CurDis);
		//mCameraMovementComponent.Get()->ZoomCameraAndMoveToViewportPosition(PreDis - CurDis, (CurTouch1 + CurTouch2) / 2);

	}

	mSecondTouch = true;
	mPreSecondTouch = CurTouch;
}

void ACombatCameraPawn::SecondTouchCompleted(const FInputActionValue& Value)
{
	mSecondTouch = false;
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

