// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraMovementComponent.generated.h"

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

	/*
	* @brief 클램핑 박스 중앙 위치
	* @details
	* 해당 위치를 중심으로 클램핑 박스가 설정됩니다.
	*/
	UPROPERTY(Category = CameraMove, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MoveClampingBoxCenter", AllowPrivateAccess = "true"))
	FVector mMoveClampingBoxCenter = FVector(0, 0, 0);
	
	/*
	* @brief 클램핑 박스
	* @details
	* 박스 크기로 해당 카메라는 해당 위치를 벗어나지 못합니다.
	* Z축은 사용하지 않습니다.
	*/
	UPROPERTY(Category = CameraMove, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MoveClampingBox", AllowPrivateAccess = "true"))
	FVector2D mMoveClampingBox = FVector2D(1000, 1000);

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
	void SetMaxOrthoWidth(float MaxOrthoWidth);

	UFUNCTION(BlueprintCallable)
	void SetMinOrthoWidth(float MinOrthoWidth);

	UFUNCTION(BlueprintCallable)
	void SetMoveClampingBoxCenter(FVector MoveClampingBoxCenter);

	UFUNCTION(BlueprintCallable)
	void SetMoveClampingBox(FVector2D MoveClampingBox);

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
	void ZoomCameraAndMoveToViewportPosition(float ZoomValue, FVector2D ViewPortPos);

	/*
	* @brief MoveToViewportPosition로 카메라의 시선을 옮긴다.
	* @defatils
	* MoveToViewportPosition에서 Ray를 쏜다음 충돌한 위치로 카메라의 시선을 옮깁니다.
	* @param MoveToViewportPosition에서 Ray를 쏜 다음 충돌한 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToViewportPosition(FVector2D ViewPortPos);

	/*
	* @brief WorldPosition로 카메라의 시선을 옮긴다.
	* @defatils
	* WorldPosition로 카메라의 시선을 옮깁니다.
	* @param WorldPosition 위치로 카메라의 시선 옮깁니다.
	*/
	UFUNCTION(BlueprintCallable)
	void MoveToWorldPosition(FVector WorldPosition);

	//void SkillMotionZoomIn() {};

private:
	 
	/*
	* @brief 카메라가 있는 위치를 기준으로 Ray를 쏘고 결과를 받습니다.
	* @param HitResult 결과를 반환합니다.
	* @return 성공 여부를 반환합니다.
	*/
	bool GetCameraRayHitPoint(OUT FHitResult& HitResult);


};
