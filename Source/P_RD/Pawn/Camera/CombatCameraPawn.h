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
	float LocationX;
	float LocationY;
	bool bIsCurrentlyPressed = false;
};


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

	//@brief 이전 틱의 Touch 위치와 상태 변수
	FTouchState mPreTouchState1;

	//@brief 이전 틱의 Touch 위치와 상태 변수
	FTouchState mPreTouchState2;

	// ==============================================
	// 테스트용 변수
	bool mFirstTouch;
	bool mSecondTouch;
	FVector2D mPreFirstTouch;
	FVector2D mPreSecondTouch;


public:
	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent();

	UFUNCTION(BlueprintCallable)
	UCameraMovementComponent* GetCameraMovementComponent();

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

private:
	// @brief 터치 한 위치로 카메라의 시선을 이동 시키는 함수
	void TouchMoveKey(const FInputActionValue& Value);

	// @brief 지속적으로 Zoom을 하는 함수
	void ZoomKey(const FInputActionValue& Value);
	
	// @brief 틱 상태를 false로 되돌립니다.
	void ZoomEndKey(const FInputActionValue& Value);

	// @brief 틱 상태를 false로 되돌립니다.
	void TouchTragMoveKey(const FInputActionValue& Value);

	// ==================================================
	// Zoom 테스트
	void FirstTouchStart(const FInputActionValue& Value);
	void FirstTouchCompleted(const FInputActionValue& Value);

	void SecondTouchStart(const FInputActionValue& Value);
	void SecondTouchCompleted(const FInputActionValue& Value);
};
