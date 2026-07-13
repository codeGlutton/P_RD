// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/ActorView.h"
#include "GameFramework/Actor.h"
#include "Obstacle.generated.h"

class UObstacleModel;

UCLASS(abstract)
class P_RD_API AObstacle : public AActor, public IActorView
{
	GENERATED_BODY()
	
public:
	AObstacle();

	/* IActorView 상속 */
	// @brief 이동 델리게이트 구독
	void BindModel(UObjectModel* Model) override;
	// @brief 이동 델리게이트 구독 해제
	void UnbindModel(UObjectModel* Model) override;

protected:
	UObjectModel* GetModel_Internal() const override;

	// @brief 배치 요청을 수신
	virtual void OnPlaceTileTransform(const FTileTransform& TileTransform, const FTransform& Transform);

public:
	UCapsuleComponent* GetCapsuleComponent() const;
	UStaticMeshComponent* GetMesh() const;

	UArrowComponent* GetArrowComponent() const;

private:
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CapsuleComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> mCapsuleComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> mMeshComp;

	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ArrowComp", AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> mArrowComp;

protected:
	TWeakObjectPtr<UObstacleModel> mObstacleModel;

};
