// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/CameraMovementComponent/CameraMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Pawn/Camera/CombatCameraPlane.h"
#include "GameFramework/SpringArmComponent.h"

// @note checkf(Camera) 할 것

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

	// 초기 시작 위치를 중심으로 설정합니다.
	mMoveClampingBoxCenter = GetOwner()->GetActorLocation();
	mMoveClampingBoxCenter.Z = 0.f;

	// ACombatCameraPlane을 생성합니다.
	// ACombatCameraPlane은 카메라 조작을 위해서 반드시 필요한 액터입니다
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 카메라보다 아래에 카메라판을 생성합니다.
	FVector SpawnPos = GetOwner()->GetActorLocation();
	SpawnPos.Z = -500;

	mCameraPlane = GetWorld()->SpawnActor<ACombatCameraPlane>(
		ACombatCameraPlane::StaticClass(),
		SpawnPos,
		FRotator::ZeroRotator,
		SpawnParams
	);
}

void UCameraMovementComponent::InitializeCameraTargetLocation()
{
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector2D CurLocation = FVector2D(CameraCenterRayCastHitResult.ImpactPoint);

	mTargetLocation = CurLocation;
	mInitCameraLocation = true;
}

void UCameraMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 카메라가 제거 될 때 판도 같이 제거합니다.
	if (mCameraPlane.IsValid())
	{
		mCameraPlane->Destroy();
		mCameraPlane = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 강조가 완전히 끝났는지 판단하는 로직입니다.
	// Emphasis_Returning 중이고 현재 위치가 타겟 위치로 완전히 돌아왔다면 기본 상태로 되돌립니다.
	if (mCamerControlState == ECameraControlState::Emphasis_Returning)
	{
		// 도착 판정: 여기서만 임계값을 쓰되, 목적은 "재진입 허용 시점" 판단이지
		// "저장할 위치가 정확한지" 판단이 아니므로 훨씬 안전함
		if (FVector2D::Distance(mCurCameraLocation, mTargetLocation) < 1.f)
		{
			mCamerControlState = ECameraControlState::Normal;
		}
	}

	// 카메라의 위치를 초기화합니다.
	// 한번만 실행 되어야 합니다.
	if (!mInitCameraLocation)
	{
		InitializeCameraTargetLocation();
	}

	MoveSmooth(DeltaTime);		// 카메라를 이동시키는 함수입니다.
	FollowActor();				// 강조 시 액터가 있다면 액터를 따라가는 함수입니다.
	ClampingCamera();			// 카메라의 Location, OrthoWidth를 제한하는 함수입니다.
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

//void UCameraMovementComponent::SetZoomSpeed(float ZoomSpeed)
//{
//	mZoomSpeed = ZoomSpeed;
//}

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

#pragma region Zoom

void UCameraMovementComponent::ZoomCamera_Instant(float ZoomDelta)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));
	mCameraComponent->OrthoWidth = FMath::Clamp(mCameraComponent->OrthoWidth + ZoomDelta, mMinOrthoWidth, mMaxOrthoWidth);
}

void UCameraMovementComponent::ZoomCamera_InstantAndMoveToViewportPosition_Instant(float ZoomDelta, FVector2D ViewPortPos)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 확대 전 ViewPortPos 구하기
	// =========================================
	// ViewPortPos를 World 좌표로 변환합니다.
	FVector WorldLocation;
	FVector WorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	FHitResult ViewPortHitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams,
		QueryParams
	);

	// 충돌되지 않았다면 취소
	if (!ensureMsgf(bViewPortHit, TEXT("ViewPortPos에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector PrePos = ViewPortHitResult.ImpactPoint;
	PrePos.Z = 0;

	// ===============================================================

	// 카메라를 줌 한다.
	ZoomCamera_Instant(ZoomDelta);

	// 카메라를 갱신하여 ProjMat을 갱신해줍니다.
	GetWorld()->GetFirstPlayerController()->PlayerCameraManager->UpdateCamera(0.0f);

	// 확대 후 ViewPortPos 구하기
	// =========================================
	// ViewPortPos를 World 좌표로 변환합니다.
	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	StartTrace = WorldLocation; // 시작 지점
	EndTrace = StartTrace + (WorldDirection * 100000.0f);

	bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams,
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("ViewPortPos에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector NextPos = ViewPortHitResult.ImpactPoint;
	NextPos.Z = 0;

	// ============================================================

	// 위치의 차를 이용하여 카메라의 위치를 옮깁니다.
	GetOwner()->AddActorWorldOffset(PrePos - NextPos);
}

void UCameraMovementComponent::ZoomCamera_Smooth(float TargetZoom)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 0.01초 간격으로 호출할 예정이므로 (Duration / 0.01)번 반복
	mStartZoom = mCameraComponent->OrthoWidth;
	mCurZoom = mStartZoom;
	mEndZoom = TargetZoom;

	// 0.01초 간격으로 호출할 예정이므로 (Duration / 0.01)번 반복
	mCurrentZoomAlpha = 0.0f;

	// Timer로 Zoom을 수행합니다.
	GetWorld()->GetTimerManager().SetTimer(mTimerHandle_Zoom, this, &UCameraMovementComponent::ZoomSmooth, 0.01f, true);
}

void UCameraMovementComponent::ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(float TargetZoom, FVector2D ViewPortPos)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	ZoomCamera_Smooth(TargetZoom);
	MoveToViewportPosition_Smooth(ViewPortPos);
}

void UCameraMovementComponent::ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(float TargetZoom, FVector WorldPosition)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	ZoomCamera_Smooth(TargetZoom);
	MoveToWorldPosition_Smooth(WorldPosition);
}

#pragma endregion

#pragma region Move

void UCameraMovementComponent::MoveToViewportPosition_Instant(FVector2D ViewPortPos)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	//==============================
	// ViewPortPos를 WorldLocation으로 변환하고 Lay를 쏩니다.
	FVector WorldLocation;
	FVector WorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	FHitResult ViewPortHitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ============================================
	// 카메라의 시선으로로 Ray를 쏜다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ==========================================
	// 카메라의 위치를 이동시킨다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector LocationPos = ViewPortHitResult.ImpactPoint;
	FVector CameraPos = CameraCenterRayCastHitResult.ImpactPoint; // 카메라가 바라보고 있는 땅의 위치를 가져온다.
	LocationPos.Z = 0;
	CameraPos.Z = 0;

	mTargetLocation = FVector2D(LocationPos);
	GetOwner()->AddActorWorldOffset(LocationPos - CameraPos);
}

void UCameraMovementComponent::MoveToWorldPosition_Instant(FVector WorldPosition)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// LocationPos를 카메라에 투영하고 다시 Raycast를 계산한다.
	FVector2D ViewPort;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewPort);

	MoveToViewportPosition_Instant(ViewPort);
}

void UCameraMovementComponent::MoveToViewportPosition_Smooth(FVector2D ViewPortPos)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	//==============================
	// ViewPortPos를 WorldLocation으로 변환하고 Lay를 쏩니다.
	FVector WorldLocation;
	FVector WorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(ViewPortPos.X, ViewPortPos.Y, WorldLocation, WorldDirection);

	FHitResult ViewPortHitResult;
	FVector StartTrace = WorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (WorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("ViewPort에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ============================================
	// 카메라의 시선으로로 Ray를 쏜다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	// ==========================================
	// 카메라의 최종 위치를 변경합니다.
	// Z 축은 반영하지 않으므로 0으로 만든다.
	FVector LocationPos = ViewPortHitResult.ImpactPoint;
	FVector CameraPos = CameraCenterRayCastHitResult.ImpactPoint; // 카메라가 바라보고 있는 땅의 위치를 가져온다.
	LocationPos.Z = 0;
	CameraPos.Z = 0;

	mTargetLocation = FVector2D(LocationPos);
}

void UCameraMovementComponent::MoveToWorldPosition_Smooth(FVector WorldPosition)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// WorldPosition를 카메라에 투영하고 다시 Raycast를 계산한다.
	FVector2D ViewPort;
	GetWorld()->GetFirstPlayerController()->ProjectWorldLocationToScreen(WorldPosition, ViewPort);


	MoveToViewportPosition_Smooth(ViewPort);
}

void UCameraMovementComponent::DragMoveToViewportPosition_Instant(FVector2D PreViewPortPos, FVector2D CurViewPortPos)
{
	//if (mCamerControlState != ECameraControlState::Normal)
	//	return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FVector PreWorldLocation;
	FVector PreWorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(PreViewPortPos.X, PreViewPortPos.Y, PreWorldLocation, PreWorldDirection);

	FHitResult ViewPortHitResult;
	FVector StartTrace = PreWorldLocation; // 시작 지점
	FVector EndTrace = StartTrace + (PreWorldDirection * 100000.0f);

	// CameraMove Channel을 가진 오브젝트와 충돌 체크를 진행합니다.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("Viewport에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector PrePos = ViewPortHitResult.ImpactPoint;
	PrePos.Z = 0;

	//======================================
	FVector CurWorldLocation;
	FVector CurWorldDirection;

	GetWorld()->GetFirstPlayerController()->DeprojectScreenPositionToWorld(CurViewPortPos.X, CurViewPortPos.Y, CurWorldLocation, CurWorldDirection);

	//ViewPortHitResult;
	StartTrace = CurWorldLocation; // 시작 지점
	EndTrace = StartTrace + (CurWorldDirection * 100000.0f);

	// 채널 설정 (ECC_Visibility 등)
	//QueryParams;
	//QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bViewPortHit = GetWorld()->LineTraceSingleByObjectType(
		ViewPortHitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	if (!ensureMsgf(bViewPortHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	FVector CurPos = ViewPortHitResult.ImpactPoint;
	CurPos.Z = 0;
	
	// 목표 위치와 현재 카메라 위치를 같이 변경합니다.
	mTargetLocation += FVector2D(PrePos - CurPos);
	GetOwner()->AddActorWorldOffset(PrePos - CurPos);
}

#pragma endregion

#pragma region Emphasis
void UCameraMovementComponent::StartEmphasisToWorldPositionWithZoom(float TargetZoom, FVector WorldPosition)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(TargetZoom, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPositionWithZoom(float TargetZoom, FVector2D ViewPortPos)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(TargetZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;

}

void UCameraMovementComponent::StartEmphasisToActorWithZoom(float TargetZoom, AActor* EmphasisActor)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 액터의 현재 위치를 구합니다.
	FVector WorldPosition = EmphasisActor->GetActorLocation();

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(TargetZoom, WorldPosition);

	mEmphasisActor = EmphasisActor;

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToWorldPosition(FVector WorldPosition)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mEmphasisZoom, WorldPosition);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToViewPortPosition(FVector2D ViewPortPos)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(mEmphasisZoom, ViewPortPos);

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::StartEmphasisToActor(AActor* EmphasisActor)
{
	if (mCamerControlState == ECameraControlState::Emphasis)
		return;

	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	FHitResult CameraRayHitResult;
	// 카메라의 현재 상태를 저장합니다.
	if (GetCameraRayHitPoint(CameraRayHitResult))
	{
		// 아직 타이머가 작동 중이라면 이전값을 유지합니다.
		mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
		mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
			mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
	}

	// 액터의 현재 위치를 구합니다.
	FVector WorldPosition = EmphasisActor->GetActorLocation();

	// 해당 위치로 Zoom과 이동을 수행합니다.
	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mEmphasisZoom, WorldPosition);

	mEmphasisActor = EmphasisActor;

	mCamerControlState = ECameraControlState::Emphasis;
}

void UCameraMovementComponent::FocusToWorldPosition(const FVector& WorldPosition)
{
	if (!ensureMsgf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다")))
	{
		return;
	}

	// 처음 옮길 때만 지금 화면을 기억한다.
	if (mCamerControlState != ECameraControlState::Emphasis)
	{
		FHitResult CameraRayHitResult;
		if (GetCameraRayHitPoint(CameraRayHitResult))
		{
			mPreDefaultState.Position = mCamerControlState == ECameraControlState::Emphasis_Returning ?
				mPreDefaultState.Position : CameraRayHitResult.ImpactPoint;
			mPreDefaultState.Zoom = mCamerControlState == ECameraControlState::Emphasis_Returning ?
				mPreDefaultState.Zoom : mCameraComponent->OrthoWidth;
		}
		mCamerControlState = ECameraControlState::Emphasis;
	}

	// 따라갈 액터는 없다. 남아 있으면 FollowActor() 가 그쪽으로 끌어당긴다.
	mEmphasisActor.Reset();

	// 확대는 안 건드린다. 가운데로 놓는 것만 한다.
	MoveToWorldPosition_Smooth(WorldPosition);
}

void UCameraMovementComponent::EndEmphasis()
{
	// 카메라의 강조 상태를 종료합니다.
	mCamerControlState = ECameraControlState::Emphasis_Returning;

	mEmphasisActor = nullptr;

	ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(mPreDefaultState.Zoom, mPreDefaultState.Position);
}

void UCameraMovementComponent::FollowActor()
{
	if (!ensureMsgf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다")))
	{
		return;
	}

	if (mCamerControlState != ECameraControlState::Emphasis)
	{
		return;
	}

	if (!mEmphasisActor.IsValid())
	{
		return;
	}

	FVector WorldPosition = mEmphasisActor->GetActorLocation();

	MoveToWorldPosition_Smooth(WorldPosition);
}

#pragma endregion

void UCameraMovementComponent::StartCameraShake(TSubclassOf<class UCameraShakeBase> CameraShakeClass)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	APlayerController* FirstPlayerController = GetWorld()->GetFirstPlayerController();
	checkf(FirstPlayerController != nullptr, TEXT("PlayerController가 없습니다"));

	if (!ensureMsgf(CameraShakeClass != nullptr, TEXT("Camera Shake Class가 존재하지 않습니다.")))
	{
		return;
	}

	FirstPlayerController->PlayerCameraManager->StartCameraShake(CameraShakeClass);
}

bool UCameraMovementComponent::GetCameraRayHitPoint(OUT FHitResult& HitResult)
{
	FVector StartTrace = GetOwner()->GetActorLocation(); // 시작 지점
	FVector EndTrace = StartTrace + (GetOwner()->GetActorForwardVector() * 100000.0f);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_GameTraceChannel3);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // 자기 자신은 충돌에서 제외

	bool bHit = GetWorld()->LineTraceSingleByObjectType(
		HitResult,
		StartTrace,
		EndTrace,
		ObjectParams, // 충돌 채널
		QueryParams
	);

	return bHit;
}

void UCameraMovementComponent::ClampingCamera()
{
	if (!ensureMsgf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다")))
	{
		return;
	}

	// =============================
	// 확대, 축소 최소 최대 Clamping
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
	// @note 
	// 직교 카메라로 보고 있을 때는 제한 범위가 이상해 보일 것입니다.
	// 에디터 카메라로 보아야지 제한 범위가 납득이 되실 것입니다.
	FVector Center = mMoveClampingBoxCenter;
	FVector Extent = FVector(mMoveClampingBox / 2, 0);
	FQuat Rotation = FRotator(0.f, GetOwner()->GetActorRotation().Yaw, 0.f).Quaternion();

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

void UCameraMovementComponent::MoveSmooth(float DeltaTime)
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// 카메라의 시선 위치를 구합니다.
	FHitResult CameraCenterRayCastHitResult;
	bool bCameraHit = GetCameraRayHitPoint(CameraCenterRayCastHitResult);

	// 카메라의 중심에서 Ray가 적중하지 않았다면 이동시키지 않는다.
	if (!ensureMsgf(bCameraHit, TEXT("카메라에서 쏜 Ray가 CameraPlane과 닿지 않았습니다.")))
	{
		return;
	}

	mCurCameraLocation = FVector2D(CameraCenterRayCastHitResult.ImpactPoint);
	FVector DeltaLocation = FVector((mTargetLocation - mCurCameraLocation) * DeltaTime * 5, 0);
	GetOwner()->AddActorWorldOffset(DeltaLocation);
}

void UCameraMovementComponent::ZoomSmooth()
{
	checkf(mCameraComponent.IsValid(), TEXT("카메라가 유효하지 않습니다"));

	// mZoomDuration 시간동안 줌을 합니다.
	mCurrentZoomAlpha += 0.01f / mZoomDuration; // 시간에 따른 진행도 증가

	if (mCurrentZoomAlpha >= 1.0f)
	{
		mCurrentZoomAlpha = 1.0f;
		GetWorld()->GetTimerManager().ClearTimer(mTimerHandle_Zoom); // 타이머 종료
	}

	// 선형 보간 적용
	float NewZoom = FMath::InterpEaseInOut(mStartZoom, mEndZoom, mCurrentZoomAlpha, mZoomExp);

	mCameraComponent->OrthoWidth = FMath::Clamp(NewZoom, mMinOrthoWidth, mMaxOrthoWidth);

}
