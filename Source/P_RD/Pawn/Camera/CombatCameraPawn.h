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

private:
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

public:
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

	/**
	 * @brief 전투 모달 UI가 떠 있는 동안 raw touch 기반 카메라 제스처를 막는다.
	 *
	 * @details 이 Pawn은 UMG의 Handled 여부와 무관하게 GetInputTouchState를 직접
	 * 폴링한다. 따라서 용병/인벤토리/몬스터/상세 팝업이 입력을 소비해도 여기서
	 * 별도로 잠그지 않으면 손가락 움직임이 카메라 드래그로 새어 들어온다.
	 */
	UFUNCTION(BlueprintCallable)
	void SetTouchGestureInputEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure)
	bool IsTouchGestureInputEnabled() const { return mTouchGestureInputEnabled; }

private:
	/*
	* @brief Drag 중인지 나타내는 함수
	* @return true 시 드래그 중, false 시 드래그 아님
	*/
	bool IsDrag();

	/*
	* @brief Pinch 중인지 나타내는 함수
	* @return true 시 Pinch 중, false 시 Pinch 아님
	*/
	bool IsPinch();

private:
	/** UMG 모달이 켜지면 false. raw touch 폴링 자체를 멈춘다. */
	bool mTouchGestureInputEnabled = true;

	/*
	* @brief Drag 중 카메라가 시행할 행동 
	*/
	void Dragging(const TArray<FTouchState>& TouchState);

	/*
	* @brief Pinch 중 카메라가 시행할 행동
	*/
	void Pinching(const TArray<FTouchState>& TouchState);
};
