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


/*
* @brief 카메라 강조 상태 
* @details
* 기본 상태 -> 강조 중 -> 강조 해제 중 -> 기본 상태 
* 기본 상태 -> 강조 중 -> 강조 해제 중 -> 강조 중 도 가능
*/
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECameraControlState : uint8
{
	Normal,					// 기본 상태
	Emphasis,				// 강조 중
	Emphasis_Returning,		// 강조 해제 중
};

/*
* @brief 카메라 강조 중 정보 저장용
* @details
* 카메라 강조 중 이전 카메라 위치, zoom 상태를 저장하기 위해 만든 구조체
*/
struct FCameraEmphasisState
{
	FVector		Position;
	float		Zoom;
};

class UCameraComponent;
class USpringArmComponent;

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
	float mMinOrthoWidth = 750.f;

	UPROPERTY(Category = Zoom, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SmoothZoomSpeed", AllowPrivateAccess = "true"))
	float mZoomSpeed = 5.f;

	UPROPERTY(Category = Zoom, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CurrentZoom", AllowPrivateAccess = "true"))
	float mCurZoom;

	UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TargetZoom", AllowPrivateAccess = "true"))
	float mTargetZoom;


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
	* @brief 이동 속도
	* @details
	* Smoooth 이동 시 속도
	* 1 / MoveSmoothSpeed -> 0.5초
	*/
	UPROPERTY(Category = CameraMove, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SmoothMoveSpeed", AllowPrivateAccess = "true"))
	float mMoveSpeed = 5.f; // 이동 속도 

	/*
	* @brief 현재 카메라가 바라보고 있는 시선 위치
	*/
	UPROPERTY(Category = CameraMove, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "LookAtCameraLocation", AllowPrivateAccess = "true"))
	FVector2D mCurrentLookAtCameraLocation;

	/*
	* @brief 카메라가 최종적으로 바라볼 시선 위치
	*/
	UPROPERTY(Category = CameraMove, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TargetLookAtCameraLocation", AllowPrivateAccess = "true"))
	FVector2D mTargetLookAtCameraLocation;
	

protected:
	/* 강조 */

	/*
	* @brief 현재 강조되고 있는 액터
	* @details 액터가 존재하며, ECameraControlState가 Emphasis 상태라면 액터 위치에 시선을 고정시킵니다.
	*/
	UPROPERTY(Category = Emphasis, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "강조된 액터", AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor> mEmphasisActor;

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

protected:
	UPROPERTY(Category = CameraShake, VisibleAnywhere,BlueprintReadOnly, meta = (DisplayName = "현재 카메라 셰이크 인스턴스", AllowPrivateAccess = "true"))
	TObjectPtr<class UCameraShakeBase> mActiveCameraShakeInstance;


private:
	/*
	* @brief 카메라 초기화
	* @details
	* CombatCameraPlane 설치 후 딱 한번만 초기화를 진행할 때 사용하는 변수
	*/
	bool mInitCameraLocation = false;

	/* 기타 내부 함수*/
private:
	/*
	* @brief mTargetLocation을 초기화합니다.
	* @details Plane 생성 후 현재 위치에서 Ray를 쏘아 mTargetLocation을 초기화합니다.
	*/
	void InitializeCameraTargetLocation();

	/*
	* @brief 카메라가 있는 위치를 기준으로 Ray를 쏘고 결과를 받습니다.
	* @param HitResult 결과를 반환합니다.
	* @return 성공 여부를 반환합니다.
	*/
	bool GetCameraRayHitPoint(OUT FHitResult& HitResult);

	FVector ClampLocationWithinRotatedBox(const FVector& WorldLocation);

	/*
	* @brief 카메라의 위치와 OrthoWidth의 크기를 범위에서 벗어나지 못하게 합니다.
	*/
	void ClampingCamera();

	/*
	* @brief 이동 시 매 틱마다 호출하는 함수
	* @details
	* 카메라의 위치를 TargetLocation으로 부드럽게 이동시킵니다.
	* 틱에서 호출됩니다.
	*/
	void MoveSmooth(float DeltaTime);

	/*
	* @brief 줌 시 매 틱마다 호출하는 함수
	* @details
	* 카메라의 zoom을 mEndZoom으로 부드럽게 변경시킵니다.
	* 타이머로 호출합니다.
	*/
	void ZoomSmooth(float DeltaTime);

	/*
	* @breif 카메라 Shake 시 OrthoWidth를 조작하는 함수
	* @details 
	* 현재 활성화된 CameraShake에서 OrthoWidthValue를 가져와 Zoom을 흔듭니다.
	*/
	void ShakeZoom();

	/*Get, Set*/
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


	/* 터치로 카메라 조작*/
public:

	/*
	* @brief ZoomDelta과 ViewPort 위치 값을 받으면 해당 위치로 Zoom하며 ViewPortPos에 맞는 위치로 이동합니다
	* @details
	* Pinch 조작 시 확대 및 이동에 사용합니다.
	* @param ZoomDelta 만큼 즉시 Zoom 합니다
	* @param ViewPortPos를 WorldPos로 변환하고 즉시 이동합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void PinchZoomCamera_InstantAndMoveToViewportPosition_Instant(float TargetZoom, FVector2D ViewPortPos);

	/*
	* @brief CurViewPortPos로 카메라의 시선을 즉시 옮긴다.
	* @defatils
	* PreViewPortPos, CurViewPortPos에서 Ray를 쏜 다음 위치 차이를 구하여 카메라의 시선을 즉시 옮깁니다.
	* Drag로 화면을 이동시킬 때 사용합니다.
	* @param CurViewPortPos 카메라의 시선을 즉시 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void DragMoveToViewportPosition_Instant(FVector2D PreViewPortPos, FVector2D CurViewPortPos);

	/* 강조 기능*/
public:
	/*
	* @brief WorldPosition로 카메라의 시선을 옮기고 TargetZoom만큼 Zoom 합니다.
	* @details Emphasis 상태일 때는 다른 카메라 조작(이동, 줌)을 무시합니다.
	* @param WorldPosition 위치로 카메라의 시선 옮깁니다.
	* @param TargetZoom 만큼 Zoom 합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToWorldPositionWithZoom(float TargetZoom, FVector WorldPosition);

	/*
	* @brief ViewPortPosition로 카메라의 시선을 옮기고 TargetZoom만큼 Zoom 합니다.
	* @details Emphasis 상태일 때는 다른 카메라 조작(이동, 줌)을 무시합니다.
	* @param ViewPortPosition 위치로 카메라의 시선 옮깁니다.
	* @param TargetZoom 만큼 Zoom 합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToViewPortPositionWithZoom(float TargetZoom, FVector2D ViewPortPos);

	/*
	* @brief Actor로 카메라의 시선을 옮기고 Zoom값을 mEmphasisZoom로 변경합니다.
	* @details Emphasis 상태일 때는 다른 카메라 조작(이동, 줌)을 무시합니다.
	* @param EmphasisActor를 등록하여 카메라가 해당 액터를 따라갑니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToActorWithZoom(float TargetZoom, AActor* EmphasisActor);

	/*
	* @brief WorldPosition로 카메라의 시선을 옮기고 Zoom값을 mEmphasisZoom로 변경합니다.
	* @details Emphasis 상태일 때는 다른 카메라 조작(이동, 줌)을 무시합니다.
	* @param WorldPosition 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToWorldPosition(FVector WorldPosition);

	/*
	* @brief ViewPortPosition로 카메라의 시선을 옮기고 Zoom값을 mEmphasisZoom로 변경합니다.
	* @details Emphasis 상태일 때는 다른 카메라 조작(이동, 줌)을 무시합니다.
	* @param ViewPortPosition 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToViewPortPosition(FVector2D ViewPortPos);

	/*
	* @brief Actor로 카메라의 시선을 옮기고 Zoom값을 mEmphasisZoom로 변경합니다.
	* @details Emphasis 상태일 때는 다른 카메라 조작(이동, 줌)을 무시합니다.
	* @param EmphasisActor를 등록하여 카메라가 해당 액터를 따라갑니다.
	*/
	UFUNCTION(BlueprintCallable)
	void StartEmphasisToActor(AActor* EmphasisActor);

	/*
	* @brief Emphasis 상태를 종료하고 원래 카메라 위치, Zoom 값으로 변경합니다.
	*/
	UFUNCTION(BlueprintCallable)
	void EndEmphasis();

	/*
	* @brief 카메라를 그 자리로 **바로** 옮긴다(따라가는 연출 없음).
	* @details 화면(UI)에서 "이 유닛을 보여 줘" 할 때 쓴다. 부드럽게 따라가면
	*          판이 흐르듯 움직여 어지럽다는 검수가 있었다(0806).
	*          강조와 달리 카메라를 잠그지 않는다 -- 옮긴 뒤 손으로 계속 본다.
	* @param WorldPosition 이 자리가 화면 가운데 오게 옮긴다
	*/
	UFUNCTION(BlueprintCallable)
	void JumpToWorldPosition(FVector WorldPosition);

	/* 카메라 셰이크*/
public:
	UFUNCTION(BlueprintCallable)
	void StartCameraShake(TSubclassOf<class UCameraShakeBase> CameraShakeClass);

	/*카메라 조작 내부 로직*/
	/* Zoom */
private:
	/*
	* @brief ZoomDelta 값을 받으면 즉시 카메라를 해당 값만큼 Zoom 합니다
	*
	* @param ZoomDelta 만큼 즉시 Zoom 합니다
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCamera_Instant(float ZoomDelta);

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

	/* Move */
private:
	/*
	* @brief ViewPortPos로 카메라의 시선을 즉시 옮긴다.
	* @details
	* ViewPortPos에서 Ray를 쏜다음 충돌한 위치로 카메라의 시선을 천천히 옮깁니다.
	* @param ViewPortPos에서 Ray를 쏜 다음 충돌한 위치로 카메라의 시선 천천히 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToViewportPosition_Instant(FVector2D ViewPortPos);

	/*
	* @brief WorldPosition로 카메라의 시선을 즉시 옮긴다.
	* @defatils
	* WorldPosition로 카메라의 시선을 즉시 옮깁니다.
	* @param WorldPosition 위치로 카메라의 시선 즉시 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToWorldPosition_Instant(FVector WorldPosition);

	/*
	* @brief MoveToViewportPosition로 카메라의 시선을 천천히 옮긴다.
	* @defatils
	* ViewPortPos에서 Ray를 쏜다음 충돌한 위치로 카메라의 시선을 천천히 옮깁니다.
	* @param MoveToViewportPosition에서 Ray를 쏜 다음 충돌한 위치로 카메라의 시선 천천히 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToViewportPosition_Smooth(FVector2D ViewPortPos);

	/*
	* @brief WorldPosition로 카메라의 시선을 천천히 옮긴다.
	* @param WorldPosition 위치로 카메라의 시선을 천천히 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToWorldPosition_Smooth(FVector WorldPosition);

	UFUNCTION(BlueprintCallable)
	void MoveToDeltaPosition_Instant(FVector DeltaPosition);

private:
	/* 강조 기능 : private */

	/*
	* @brief 강조 상태에서 EmphasisActor가 존재한다면 TargetLocation을 조정합니다.
	* @details
	* 틱에서 호출되며 유닛의 위치를 매 틱마다 따라갈 수 있도록 TargetLocation을 조정하는 함수입니다.
	*/
	void FollowActor();

};
