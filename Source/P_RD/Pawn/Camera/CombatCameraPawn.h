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

	/*
	* @brief Drag 중 카메라가 시행할 행동 
	*/
	void Dragging(const TArray<FTouchState>& TouchState);

	/*
	* @brief Pinch 중 카메라가 시행할 행동
	*/
	void Pinching(const TArray<FTouchState>& TouchState);
};
