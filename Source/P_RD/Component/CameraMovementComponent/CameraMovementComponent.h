/*****************************************************************//**
 * @file   CameraMovementComponent.h
 * @brief  카메라 조작 로직 컴포넌트
 * @author 김준형
 * @date   2026-06-29
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraMovementComponent.generated.h"

class ACombatCameraPlane;


// @brief 카메라 강조 Handle
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECameraControlState : uint8
{
	Normal,
	Emphasis
};

struct FCameraEmphasisState
{
	FVector		Position;
	float		Zoom;
};

class UCameraComponent;
class USpringArmComponent;

//@note Raycast 시 충돌 판정에 관하여 아직 정해진 것이 없습니다.
// 추후 입력을 넣어서 작동하게 될 때 문제가 생기면 고쳐나가도록 하겠습니다.

//@note Raycast 시 끝 거리는 우선 하드 코딩해놨습니다.
// 전투 시 카메라의 위치 해당 거리를 벗어나지는 않을 것이라고 판단되어 그냥 큰 값을 넣었습니다.

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P_RD_API UCameraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCameraMovementComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadWrite, meta = (DisplayName = "CameraComponent", AllowPrivateAccess = "true"))
	TWeakObjectPtr<UCameraComponent> mCameraComponent;

	/*
	* @breif 카메라 조작을 위한 카메라판 입니다.
	*/
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadWrite, meta = (DisplayName = "CameraPlane", AllowPrivateAccess = "true"))
	TWeakObjectPtr<ACombatCameraPlane> mCameraPlane;

	//UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SpringArmComponent", AllowPrivateAccess = "true"))
	//TObjectPtr<USpringArmComponent> mSpringArmComponent;


protected:
	/*
	* @brief 줌 속력
	* @note 사용처가 불확실하여 우선 주석 처리
	*/
	//UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadWrite, meta = (DisplayName = "ZoomSpeed", AllowPrivateAccess = "true"))
	//float mZoomSpeed = 1.f;

	/*
	* @brief 최대 OrthoWidth, 또는 최소 확대
	* @details 
	* OrthoWidth 최대 값
	* 커질수록 화면이 더 많이 축소할 수 있다.
	*/
	UPROPERTY(Category = Zoom, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MaxZoom", AllowPrivateAccess = "true"))
	float mMaxOrthoWidth = 2500.f;

	/*
	* @brief 최소 OrthoWidth, 또는 최대 확대
	* @details 
	* OrthoWidth 최소 값
	* 작을수록 화면이 더 많이 확대할 수 있다.
	*/
	UPROPERTY(Category = Zoom, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MinZoom", AllowPrivateAccess = "true"))
	float mMinOrthoWidth = 100.f;

	/*
	* @brief Zoom 시 걸리는 시간
	* @details
	* 터치로 이동 시 걸리는 시간
	*/
	UPROPERTY(Category = Zoom, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SmoothZoomDuration", AllowPrivateAccess = "true"))
	float mZoomDuration = 0.75f; // 이동에 걸릴 시간

	UPROPERTY(Category = Zoom, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SmoothZoomExp", AllowPrivateAccess = "true"))
	float mZoomExp = 2.f;

	FTimerHandle mTimerHandle_Zoom;		// Zoom 로직에 쓸 타이머 핸들
	float mStartZoom;					// 시작 Zoom
	float mCurZoom;						// 현재 Zoom
	float mEndZoom;						// 끝 Zoom
	float mCurrentZoomAlpha = 0.0f;		// Zoom 로직 진행도

protected:
	/* 클램핑 박스 제한*/
	/*
	* @brief 클램핑 박스 중앙 위치
	* @details
	* 해당 위치를 중심으로 클램핑 박스가 설정됩니다.
	*/
	UPROPERTY(Category = CameraMove, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MoveClampingBoxCenter", AllowPrivateAccess = "true"))
	FVector mMoveClampingBoxCenter = FVector(0, 0, 0);
	
	/*
	* @brief 클램핑 박스
	* @details
	* 박스 크기로 해당 카메라는 해당 위치를 벗어나지 못합니다.
	* Z축은 사용하지 않습니다.
	*/
	UPROPERTY(Category = CameraMove, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MoveClampingBox", AllowPrivateAccess = "true"))
	FVector2D mMoveClampingBox = FVector2D(10000, 10000);

	/*
	* @brief 이동 시 걸리는 시간
	* @details
	* 터치로 이동 시 걸리는 시간
	*/
	UPROPERTY(Category = CameraMove, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SmoothMoveDuration", AllowPrivateAccess = "true"))
	float mMoveDuration = 0.75f; // 이동에 걸릴 시간

	/*
	* @brief 가속도 강도
	*/
	UPROPERTY(Category = CameraMove, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SmoothMoveExp", AllowPrivateAccess = "true"))
	float mMoveExp = 2.f;

	FTimerHandle mTimerHandle_Move;		// 이동 로직에 쓸 타이머 핸들
	FVector mStartLocation;				// 이동 시작 위치
	FVector mCurLocation;				// 현재 위치
	FVector mEndLocation;				// 종료 위치
	float mCurrentMoveAlpha = 0.0f;		// 이동 진행도
	

protected:
	/* 강조 */

	/*
	* @brief 강조 시 변경되는 설정되는 줌값
	* @details
	*/
	UPROPERTY(Category = Emphasis, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "강조 시 줌 값", AllowPrivateAccess = "true"))
	float				mEmphasisZoom = 500;

	/*
	* @brief 현재 강조 상태
	*/
	UPROPERTY(Category = Emphasis, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "현재 강조 상태", AllowPrivateAccess = "true"))

	ECameraControlState mCamerControlState;

	/*
	* @brief 강조를 되돌릴 때 사용하는 위치, Zoom 변화량
	* @details
	*/
	FCameraEmphasisState mPreDefaultState;


public:
	UFUNCTION(BlueprintCallable)
	void SetCameraComponent(UCameraComponent* CameraComponent);

	//UFUNCTION(BlueprintCallable)
	//void SetSpringArmComponent(USpringArmComponent* SpringComponent);

	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent();

	//UFUNCTION(BlueprintCallable)
	//void SetZoomSpeed(float ZoomSpeed);

	UFUNCTION(BlueprintCallable)
	void SetMaxOrthoWidth(float MaxOrthoWidth);

	UFUNCTION(BlueprintCallable)
	void SetMinOrthoWidth(float MinOrthoWidth);

	UFUNCTION(BlueprintCallable)
	void SetMoveClampingBoxCenter(FVector MoveClampingBoxCenter);

	UFUNCTION(BlueprintCallable)
	void SetMoveClampingBox(FVector2D MoveClampingBox);

	UFUNCTION(BlueprintCallable)
	void SetEmphasisZoom(float EmphasisZoom);

public:

	/*
	* @brief ZoomDelta 값을 받으면 즉시 카메라를 해당 값만큼 Zoom 합니다
	*
	* @param ZoomDelta 만큼 즉시 Zoom 합니다
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCamera_Instant(float ZoomDelta);

	/*
	* @brief ZoomDelta과 ViewPort 위치 값을 받으면 해당 위치로 Zoom하며 ViewPortPos에 맞는 위치로 이동합니다
	*
	* @param ZoomDelta 만큼 즉시 Zoom 합니다
	* @param ViewPortPos를 WorldPos로 변환하고 즉시 이동합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCamera_InstantAndMoveToViewportPosition_Instant(float TargetZoom, FVector2D ViewPortPos);

	/*
	* @brief 줌 값을 받아서 천천히 카메라를 Zoom합니다
	*
	* @param Zoom을 천천히 TargetZoom으로 변경합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCamera_Smooth(float TargetZoom);

	/*
	* @brief 줌 값을 받아서 천천히 카메라를 Zoom합니다
	*
	* @param Zoom을 천천히 TargetZoom으로 변경합니다.
	* @param ViewPortPos위치로 카메라를 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCamera_SmoothAndMoveToViewportPosition_Smooth(float TargetZoom, FVector2D ViewPortPos);

	/*
	* @brief 줌 값을 받아서 천천히 카메라를 Zoom합니다
	*
	* @param Zoom을 천천히 TargetZoom으로 변경합니다.
	* @param ViewPortPos위치로 카메라를 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCamera_SmoothAndMoveToWorldPosition_Smooth(float TargetZoom, FVector WorldPosition);

	/*
	* @brief MoveToViewportPosition로 카메라의 시선을 옮긴다.
	* @defatils
	* MoveToViewportPosition에서 Ray를 쏜다음 충돌한 위치로 카메라의 시선을 옮깁니다.
	* @param MoveToViewportPosition에서 Ray를 쏜 다음 충돌한 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToViewportPosition_Smooth(FVector2D ViewPortPos);

	/*
	* @brief WorldPosition로 카메라의 시선을 옮긴다.
	* @defatils
	* WorldPosition로 카메라의 시선을 옮깁니다.
	* @param WorldPosition 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToWorldPosition_Smooth(FVector WorldPosition);

	/*
	* @brief MoveToViewportPosition로 카메라의 시선을 옮긴다.
	* @defatils
	* MoveToViewportPosition에서 Ray를 쏜다음 충돌한 위치로 카메라의 시선을 옮깁니다.
	* @param MoveToViewportPosition에서 Ray를 쏜 다음 충돌한 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void DragMoveToViewportPosition_Instant(FVector2D PreViewPortPos, FVector2D CurViewPortPos);


private:
	/* 
	* @brief 이동 시 매 틱마다 호출하는 함수
	*/
	void MoveSmooth();

	/*
	* @brief 줌 시 매 틱마다 호출하는 함수
	*/
	void ZoomSmooth();


public:
	/* 강조 기능*/
	/*
	* @brief WorldPosition로 카메라의 시선을 옮기고 TargetZoom만큼 Zoom 합니다.
	* @details Emphasis 상태로 변경하여 카메라를 옮길 수 없습니다.
	* @param WorldPosition 위치로 카메라의 시선 옮깁니다.
	* @param TargetZoom 만큼 Zoom 합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToWorldPositionWithZoom(float TargetZoom, FVector WorldPosition);

	/*
	* @brief ViewPortPosition로 카메라의 시선을 옮기고 TargetZoom만큼 Zoom 합니다.
	* @details Emphasis 상태로 변경하여 카메라를 옮길 수 없습니다.
	* @param ViewPortPosition 위치로 카메라의 시선 옮깁니다.
	* @param TargetZoom 만큼 Zoom 합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToViewPortPositionWithZoom(float TargetZoom, FVector2D ViewPortPos);

	/*
	* @brief WorldPosition로 카메라의 시선을 옮기고 Zoom값을 mEmphasisZoom로 변경합니다.
	* @details Emphasis 상태로 변경하여 카메라를 옮길 수 없습니다.
	* @param WorldPosition 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToWorldPosition(FVector WorldPosition);

	/*
	* @brief ViewPortPosition로 카메라의 시선을 옮기고 Zoom값을 mEmphasisZoom로 변경합니다.
	* @details Emphasis 상태로 변경하여 카메라를 옮길 수 없습니다.
	* @param ViewPortPosition 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToViewPortPosition(FVector2D ViewPortPos);

	/*
	* @brief Emphasis 상태를 종료하고 원래 카메라 위치, Zoom 값으로 변경합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void EndEmphasis();

public:
	/* 카메라 셰이크*/

	UPROPERTY(Category = Shake, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "카메라 흔들림", AllowPrivateAccess = "true"))
	TMap<FGameplayTag, TSubclassOf<class UCameraShakeBase>> mCameraShakeClass;

	UFUNCTION(BlueprintCallable)
	void StartCameraShake(FGameplayTag Tag);


private:
	 
	/*
	* @brief 카메라가 있는 위치를 기준으로 Ray를 쏘고 결과를 받습니다.
	* @param HitResult 결과를 반환합니다.
	* @return 성공 여부를 반환합니다.
	*/
	bool GetCameraRayHitPoint(OUT FHitResult& HitResult);


};
