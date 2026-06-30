// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraMovementComponent.generated.h"

class UCameraComponent;
class USpringArmComponent;

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

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CameraComponent", AllowPrivateAccess = "true"))
	TWeakObjectPtr<UCameraComponent> mCameraComponent;

	//UPROPERTY(Category = Camera, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SpringArmComponent", AllowPrivateAccess = "true"))
	//TObjectPtr<USpringArmComponent> mSpringArmComponent;

	/*
	* @brief 줌 속력
	*/
	UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ZoomSpeed", AllowPrivateAccess = "true"))
	float mZoomSpeed = 1.f;

	/*
	* @brief 최대 OrthoWidth, 또는 최소 확대
	* @details 
	* OrthoWidth 최대 값
	* 커질수록 화면이 더 많이 축소할 수 있다.
	*/
	UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MaxZoom", AllowPrivateAccess = "true"))
	float mMaxOrthoWidth = 10000.f;

	/*
	* @brief 최소 OrthoWidth, 또는 최대 확대
	* @details 
	* OrthoWidth 최소 값
	* 작을수록 화면이 더 많이 확대할 수 있다.
	*/
	UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MinZoom", AllowPrivateAccess = "true"))
	float mMinOrthoWidth = 100.f;

	/*추후 이동 제한 로직에서 쓸 변수*/
	//UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "DefaultPos", AllowPrivateAccess = "true"))
	//FVector mDefaultPos = FVector(0, 0, 0);
	//
	///*
	//* @brief 이동할 수 있는 거리
	//* @details
	//* 박스 크기로 이동의 최대 거리를 제한한다.
	//*/
	//UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MoveClmapBox", AllowPrivateAccess = "true"))
	//FVector2D mMoveClampBox = FVector2D(1000,1000);

public:
	UFUNCTION(BlueprintCallable)
	void SetCameraComponent(UCameraComponent* CameraComponent);

	//UFUNCTION(BlueprintCallable)
	//void SetSpringArmComponent(USpringArmComponent* SpringComponent);

	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent();

	UFUNCTION(BlueprintCallable)
	void SetZoomSpeed(float ZoomSpeed);

	UFUNCTION(BlueprintCallable)
	void SetMaxOrthoWidth(float SetMaxOrthoWidth);

	UFUNCTION(BlueprintCallable)
	void SetMinOrthoWidth(float SetMinOrthoWidth);

public:

	/*
	* @brief 줌 값을 받아서 카메라를 Zoom합니다
	*
	* @param ZoomValue 만큼 Zoom 합니다
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCamera(float ZoomValue);

	/*
	* @brief 줌 값을 받아서 카메라를 Zoom합니다.
	*
	* @param ZoomValue 만큼 Zoom 합니다
	* @param ViewPortPos위치로 카메라를 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void ZoomCameraAndMoveToViewport(float ZoomValue, FVector2D ViewPortPos);

	/*
	* @brief 터치한 방향으로 카메라를 옮긴다.
	*
	* @param ViewPortPos 위치로 카메라를 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToViewport(FVector2D ViewPortPos);

	//void SkillMotionZoomIn() {};
};
