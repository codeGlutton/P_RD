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
class USceneComponent;

struct FTouchState
{
	bool bIsCurrentlyPressed = false;
	FVector2D StartTouchPos;
	FVector2D PreTouchPos;
	FVector2D CurTouchPos;

};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDragging, const TArray<FTouchState>& /*Delta*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPinching, const TArray<FTouchState>& /*Delta*/);



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

	// ========================================
	// Touch 상태 관련 변수
	//@brief 이전 틱의 Touch 위치와 상태 변수
	TArray<FTouchState> mTouchStates;
	float mImageStabilization = 3.f;

	FOnDragging OnDragging;
	FOnPinching OnPinching;


public:
	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent();

	UFUNCTION(BlueprintCallable)
	UCameraMovementComponent* GetCameraMovementComponent();

private:
	bool IsDrag();
	bool IsPinch();

private:
	void Dragging(const TArray<FTouchState>& TouchState);
	void Pinching(const TArray<FTouchState>& TouchState);
};
