/*****************************************************************//**
 * @file   Obstacle.h
 * @brief  장애물 액터 정의 헤더
 * @author 김준형
 * @date   2026-07-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/ActorView.h"
#include "GameFramework/Actor.h"
#include "Obstacle.generated.h"

class UObstacleModel;

/**
 * @brief  장애물 액터
 */
UCLASS(abstract)
class P_RD_API AObstacle : public AActor, public IActorView
{
	GENERATED_BODY()
	
public:
	AObstacle();

	/* IActorView 상속 */
public:
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
	UArrowComponent* GetArrowComponent() const;

private:
	UPROPERTY(Category = Obstacle, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CapsuleComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> mCapsuleComp;

	UPROPERTY(Category = Obstacle, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "ArrowComp", AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> mArrowComp;

protected:
	TWeakObjectPtr<UObstacleModel> mObstacleModel;
};

/**
 * @brief  스태틱 메시 기반 장애물 액터
 */
UCLASS(abstract)
class P_RD_API AStaticMeshObstacle : public AObstacle
{
	GENERATED_BODY()

public:
	AStaticMeshObstacle();

public:
	UStaticMeshComponent* GetMesh() const;

private:
	UPROPERTY(Category = Obstacle, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> mMeshComp;
};

/**
 * @brief  스켈렡톤 메시 기반 장애물 액터
 */
UCLASS(abstract)
class P_RD_API ASkeletonObstacle : public AObstacle
{
	GENERATED_BODY()

public:
	ASkeletonObstacle();

public:
	USkeletalMeshComponent* GetMesh() const;

private:
	UPROPERTY(Category = Obstacle, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> mMeshComp;
};
