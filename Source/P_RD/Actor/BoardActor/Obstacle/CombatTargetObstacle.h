/*****************************************************************//**
 * @file   CombatTargetObstacle.h
 * @brief  전투 가능한 장애물 액터 정의 헤더
 * @author 모호재
 * @date   2026-08-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/Obstacle/Obstacle.h"
#include "Actor/BoardActor/BoardCombatTargetView.h"
#include "Actor/BoardActor/BoardSelectionTargetView.h"

#include "CombatTargetObstacle.generated.h"

class UCombatTargetObstacleModel;

class UStaticMeshSkillAnimationComponent;
class USkeletonSkillAnimationComponent;

/**
 * @brief  전투 가능한 장애물 액터
 */
UCLASS(abstract)
class P_RD_API ACombatTargetObstacle : public AObstacle, public IBoardCombatTargetView, public IBoardSelectionTargetView
{
	GENERATED_BODY()

public:
	ACombatTargetObstacle();

	/* AObstacle 상속 */
public:
	void BindModel(UObjectModel* Model) override;
	void UnbindModel(UObjectModel* Model) override;

	/* IBoardCombatTargetView 상속 */
public:
	USkillAnimationComponent* GetSkillAnimationComponent() const override;
	UCombatTargetVFXTimelineComponent* GetCombatTargetVFXTimelineComponent() const override;

	UPrimitiveComponent* GetTargetMeshComponent() const override;

	/* IBoardSelectionTargetView 상속 */
public:
	bool IsSelectable() const override;

protected:
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetVFXTimelineComp", AllowPrivateAccess = "true"))
	TObjectPtr<UCombatTargetVFXTimelineComponent> mCombatTargetVFXTimelineComp;

protected:
	TWeakObjectPtr<UCombatTargetObstacleModel> mCombatTargetObstacleModel;
};

/**
 * @brief  스태틱 메시 기반 전투가능한 장애물 액터
 */
UCLASS(abstract)
class P_RD_API AStaticMeshCombatTargetObstacle : public ACombatTargetObstacle
{
	GENERATED_BODY()

public:
	AStaticMeshCombatTargetObstacle();

	/* ACombatTargetObstacle 상속 */
public:
	USkillAnimationComponent* GetSkillAnimationComponent() const override;
	UPrimitiveComponent* GetTargetMeshComponent() const override;

public:
	UStaticMeshComponent* GetMesh() const;

private:
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SkillAnimationComp", AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshSkillAnimationComponent> mSkillAnimationComp;
	UPROPERTY(Category = Obstacle, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> mMeshComp;
};

/**
 * @brief  스켈렡톤 메시 기반 전투가능한 장애물 액터
 */
UCLASS(abstract)
class P_RD_API ASkeletonCombatTargetObstacle : public ACombatTargetObstacle
{
	GENERATED_BODY()

public:
	ASkeletonCombatTargetObstacle();

	/* ACombatTargetObstacle 상속 */
public:
	USkillAnimationComponent* GetSkillAnimationComponent() const override;
	UPrimitiveComponent* GetTargetMeshComponent() const override;

public:
	USkeletalMeshComponent* GetMesh() const;

private:
	UPROPERTY(Category = Obstacle, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SkillAnimationComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletonSkillAnimationComponent> mSkillAnimationComp;
	UPROPERTY(Category = Obstacle, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MeshComp", AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> mMeshComp;
};

