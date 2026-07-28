// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "CombatCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCameraMovementComponent;
class UTimeScaleComponent;
class USceneComponent;

struct FTouchState
{
	bool bIsCurrentlyPressed = false;
	FVector2D StartTouchPos;
	FVector2D PreTouchPos;
	FVector2D CurTouchPos;

};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDragging, const TArray<FTouchState>&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPinching, const TArray<FTouchState>&);

UCLASS()
class P_RD_API ACombatCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ACombatCameraPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UPROPERTY(Category = Default, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SceneComponent", AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> mSceneComponent;

	//UPROPERTY(Category = Default, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SceneComponent", AllowPrivateAccess = "true"))
	//TObjectPtr<USpringArmComponent> mSpringArmComponent;

	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CameraComponent", AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> mCameraComponent;

	UPROPERTY(Category = CameraMovement, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CameraMovementComponent", AllowPrivateAccess = "true"))
	TObjectPtr<UCameraMovementComponent> mCameraMovementComponent;

	UPROPERTY(Category = TimeScale, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "TimeScaleComponent", AllowPrivateAccess = "true"))
	TObjectPtr<UTimeScaleComponent> mTimeScaleComponent;

	// ========================================
	// Touch 상태 관련 변수


	/*
	* @brief 카메라 조작 시 손떨림 방지 변수
	*/
	float mImageStabilization = 3.f;

	/*
	* @brief 터치 관련 상태 저장
	*/
	TArray<FTouchState> mTouchStates;

	/** @brief 이번에 손을 댄 뒤로 끌었나. 끌었으면 뗄 때 탭으로 안 친다. */
	bool mWasDragged = false;

	/** @brief 이 거리를 넘게 움직이면 끈 것으로 본다(화면 픽셀). */
	UPROPERTY(Category = "Touch", EditAnywhere, meta = (DisplayName = "TapSlack"))
	float mTapSlack = 12.f;

	/** @brief 휠 한 칸이 바꾸는 줌 양. 부호를 뒤집으면 방향이 바뀐다. */
	UPROPERTY(Category = "Touch", EditAnywhere, meta = (DisplayName = "WheelZoomStep"))
	float mWheelZoomStep = 120.f;

	/*
	* @brief Draggin 제스쳐 사용 시 호출할 카메라 행동 대리자
	*/
	FOnDragging OnDragging;
	
	/* 
	* @brief Pinching 제스쳐 사용 시 호출할 카메라 행동 대리자
	*/
	FOnPinching OnPinching;


public:
	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent();

	UFUNCTION(BlueprintCallable)
	UCameraMovementComponent* GetCameraMovementComponent();

	UFUNCTION(BlueprintCallable)
	UTimeScaleComponent* GetTimeScaleComponent();

	/* 콘솔 치트 (개발 전용) */
public:
	/**
	 * @brief 플레이어 유닛을 지정 타일까지 경로 이동 — 콘솔에서 "RDMoveTo X Y"
	 * @details
	 * 이동 빌드 UI(MOVE 버튼·이동 포인트)를 건너뛰고,
	 * 확정 경로를 실은 이동 커맨드를 직접 발행해
	 * MoveAction부터 뷰 연출까지 실제 파이프라인으로 검증.
	 * 전투에서 빙의되는 폰이라 exec 라우팅이 보장되어 여기 둠.
	 * @note
	 * UFUNCTION(Exec) 때문에 전처리기에 못 넣고, cpp에서 전처리
	 * -> 릴리즈에서는 빈 함수로 동작
	 */
	UFUNCTION(Exec)
	void RDMoveTo(int32 X, int32 Y);

	/**
	 * @brief 플레이어 유닛의 방향 전환 — 콘솔에서 "RDRotate D" (0=Forward 1=Right 2=Backward 3=Left)
	 * @details
	 * RotateActor 논리 갱신부터 뷰 회전 연출까지 실제 파이프라인으로 검증.
	 * @note
	 * UFUNCTION(Exec) 때문에 전처리기에 못 넣고, cpp에서 전처리
	 * -> 릴리즈에서는 빈 함수로 동작
	 */
	UFUNCTION(Exec)
	void RDRotate(int32 Direction);

private:
	/*
	* @brief Drag 중인지 나타내는 함수
	* @return true 시 드래그 중, false 시 드래그 아님
	*/
	bool IsDrag();

	/**
	 * @brief 끌지 않고 떼면 톡 친 것으로 보고 월드 탭을 알린다.
	 *
	 * @details
	 * 월드 입력의 주인은 카메라다. HUD 가 눌린 순간 바로 좌표를 넘기면, 지도를
	 * 밀려고 손을 댄 것까지 선택으로 처리된다. 끌었는지 아닌지는 손을 뗄 때
	 * 알 수 있고, 그걸 아는 곳이 여기다.
	 */
	void NotifyTapIfNotDragged(int32 TouchIndex, bool bWasPressed);

	/** @brief 마우스 휠로 확대/축소. 손가락 둘을 못 쓰는 PC 를 위한 것이다. */
	void ApplyWheelZoom(APlayerController* PlayerController);

	/** @brief 사람이 직접 움직이기 시작하면 자동 추적을 그만둔다. */
	void ReleaseEmphasis();

	/*
	* @brief Pinch 중인지 나타내는 함수
	* @return true 시 Pinch 중, false 시 Pinch 아님
	*/
	bool IsPinch();

private:

	/*
	* @brief Drag 중 카메라가 시행할 행동 
	*/
	void Dragging(const TArray<FTouchState>& TouchState);

	/*
	* @brief Pinch 중 카메라가 시행할 행동
	*/
	void Pinching(const TArray<FTouchState>& TouchState);
};
