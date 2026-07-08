/*****************************************************************//**
 * @file   CombatCameraPlane.h
 * @brief  카메라 조작 시 필요한 판 액터
 * @details CameraMovementComponent가 작동하기 위해서는 해당 액터가 반드시 필요합니다.
 * @author 김준형
 * @date   2026-07-08
 *********************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatCameraPlane.generated.h"

UCLASS()
class P_RD_API ACombatCameraPlane : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACombatCameraPlane();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CameraPlane")
	TObjectPtr<UStaticMeshComponent> mPlaneMesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
