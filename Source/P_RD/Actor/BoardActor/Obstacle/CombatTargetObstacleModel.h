/*****************************************************************//**
 * @file   CombatTargetObstacleModel.h
 * @brief  전투 가능한 장애물 모델 정의 헤더
 * @author 모호재
 * @date   2026-08-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/Obstacle/ObstacleModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"

#include "CombatTargetObstacleModel.generated.h"

class UAttributeSetComponentModel;
class USkillComponentModel;
class UBoardMovementComponentModel;

class UCombatTargetAttributeSet;

/**
 * @brief  전투 가능한 장애물 모델
 */
UCLASS(abstract)
class P_RD_API UCombatTargetObstacleModel : public UObstacleModel, public IBoardCombatTarget
{
	GENERATED_BODY()

public:
	UCombatTargetObstacleModel();

	/* UObstacleModel 상속 */
public:
	void PostInitializeComponentModels() override;

public:
	void OnEndRoom() override;

	/* IBoardCombatTarget 상속 */
public:
	UAttributeSetComponentModel* GetAttributeComponentModel() const override;
	USkillComponentModel* GetSkillComponentModel() const override;
	UBoardMovementComponentModel* GetBoardMovementComponentModel() const override;

public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

public:
	void SetDifficulty(int32 Difficulty);

public:
	int32 GetDifficulty() const;

private:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	TObjectPtr<UAttributeSetComponentModel> mAttributeCompModel;

	UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "SkillCompModel"))
	TObjectPtr<USkillComponentModel> mSkillCompModel;

	UPROPERTY(Category = Movement, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "MovementCompModel"))
	TObjectPtr<UBoardMovementComponentModel> mMovementCompModel;

private:
	/** @brief 난이도 스케일 AttributeSet */
	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "CombatTargetAttributeSet"))
	TObjectPtr<UCombatTargetAttributeSet> mCombatTargetAttributeSet;

protected:
	FGenericTeamId mTeamId;

protected:
	// @brief 초기 스텟에 반영되는 난이도 수치
	UPROPERTY(Category = Combat, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;
};
