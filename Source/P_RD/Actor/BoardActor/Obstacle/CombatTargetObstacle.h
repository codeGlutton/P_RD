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
class UDissolveVFXTimelineComponent;

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
	UDissolveVFXTimelineComponent* GetDissolveVFXTimelineComponent() const override;

	UPrimitiveComponent* GetTargetMeshComponent() const override;

	/* IBoardSelectionTargetView 상속 */
public:
	bool IsSelectable() const override;

protected:
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "SkillAnimationComp", AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshSkillAnimationComponent> mSkillAnimationComp;
	UPROPERTY(Category = Unit, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "DissolveVFXTimelineComp", AllowPrivateAccess = "true"))
	TObjectPtr<UDissolveVFXTimelineComponent> mDissolveVFXTimelineComp;

protected:
	TWeakObjectPtr<UCombatTargetObstacleModel> mCombatTargetObstacleModel;
};
