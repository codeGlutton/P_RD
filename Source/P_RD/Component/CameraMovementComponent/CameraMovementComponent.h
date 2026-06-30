// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraMovementComponent.generated.h"

class UCameraComponent;

USTRUCT(BlueprintType)
struct FTouchStateContext
{
	GENERATED_BODY()

	/*
	* @brief 뷰포트 X 위치
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchState")
	float LocationX;

	/*
	* @brief 뷰포트 Y 위치
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchState")
	float LocationY;

	/*
	* @brief 터치가 되고 있는지
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TouchState")
	bool bIsCurrentlyPressed;
};

struct FMotionZoomHandle
{

};


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

	TMap<ETouchIndex::Type, FTouchStateContext> mPrevTickTouchState;

	/*
	* @brief 줌 속력
	*/
	UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ZoomSpeed", AllowPrivateAccess = "true"))
	float mZoomSpeed = 10.f;

	/*
	* @brief 최대 OrthoWidth, 또는 최소 확대
	* @details 
	* OrthoWidth 최대 값
	* 커질수록 화면이 더 많이 축소할 수 있다.
	*/
	UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MaxZoom", AllowPrivateAccess = "true"))
	float mMaxOrthoWidth = 2000.f;


	/*
	* @brief 최소 OrthoWidth, 또는 최대 확대
	* @details 
	* OrthoWidth 최소 값
	* 커질수록 화면이 더 많이 확대할 수 있다.
	*/
	UPROPERTY(Category = Zoom, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MinZoom", AllowPrivateAccess = "true"))
	float mMinOrthoWidth = 500.f;

public:
	UFUNCTION(BlueprintCallable)
	void SetCameraComponent(UCameraComponent* CameraComponent);

	UFUNCTION(BlueprintCallable)
	UCameraComponent* GetCameraComponent();

	UFUNCTION(BlueprintCallable)
	void SetZoomSpeed(float ZoomSpeed);

public:
	/*
	* @brief 줌 값을 받아서 Zoom을 실행한다.
	* 
	* @param Zoom할 값
	* @param Zoom할 중심 위치
	*/
	UFUNCTION(BlueprintCallable)
	void Zoom(float ZoomValue, FVector2D PinchCenter);

	void SkillMotionZoomIn() {};
};
