/*****************************************************************//**
 * @file   UnitModel.h
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 모델 정의 파일
 * @author 모호재
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "GenericTeamAgentInterface.h"

#include "UnitModel.generated.h"

class UUnitModel;

class UAttributeSetComponentModel;
class UUnitSkillComponentModel;
class UPassiveComponentModel;

/** @brief 이동 연출이 한 타일에 도착할 때마다 완료 칸 수를 UI 어댑터에 알린다. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	FOnMoveStepArrivedUI,
	int32 /*CompletedStepCount*/,
	int32 /*TotalStepCount*/);

/**
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 모델
 */
UCLASS(abstract, Blueprintable)
class P_RD_API UUnitModel : public UBoardActorModel, public IBoardCombatTarget
{
	GENERATED_BODY()

public:
	UUnitModel();

	/* UBoardActorModel 상속 */
public:
	void OnBeginRoom() override;
	void OnEndRoom() override;

public:
	/**
	 * @brief 자신의 턴 시작마다 실행될 함수
	 */
	virtual void OnBeginTurn(int32 TurnCount);
	virtual void OnEndTurn(int32 TurnCount);

	/* IBoardCombatTarget 상속 */
public:
	UAttributeSetComponentModel* GetAttributeComponentModel() const override;
	USkillComponentModel* GetSkillComponentModel() const override;

public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

public:
	void OnStartUsingSkill(const FActiveSkillContext& Context, int32 SkillIndex) override;
	void OnEndUsingSkill(int32 SkillIndex) override;

public:
	void OnStartApplyingEffects(const FActiveSkillContext& Context, int32 PhaseIndex) override;
	void OnEndApplyingEffects(const FActiveSkillContext& Context, int32 PhaseIndex) override;
	void OnStartReceivingEffects(UBoardCombatTargetSnapshotData* InstigatorSnapshot, const FActiveSkillContext& Context, int32 PhaseIndex) override;
	void OnEndReceivingEffects(UBoardCombatTargetSnapshotData* InstigatorSnapshot, const FActiveSkillContext& Context, int32 PhaseIndex) override;

	/* 자체 함수 */
public:
	UPassiveComponentModel* GetPassiveComponentModel() const;

public:
	virtual int32 GetDifficulty() const PURE_VIRTUAL(UUnitModel::GetDifficulty, return 0;)
	virtual bool IsPlayerUnitModel() const PURE_VIRTUAL(UUnitModel::IsPlayerUnit, return false;)

public:
	/**
	 * @brief 물리 이동 연출이 타일 하나에 도착했을 때의 진행 알림.
	 * @details 완료 칸 수는 이동 경로 시작 타일을 제외한 절대값이라 중복 알림에도
	 *          UI가 AP를 두 번 차감하지 않고 같은 표시값으로 맞출 수 있다.
	 */
	FOnMoveStepArrivedUI OnMoveStepArrivedUI;

private:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	TObjectPtr<UAttributeSetComponentModel> mAttributeCompModel;

	UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "SkillCompModel"))
	TObjectPtr<UUnitSkillComponentModel> mSkillCompModel;

	UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "PassiveCompModel"))
	TObjectPtr<UPassiveComponentModel> mPassiveCompModel;

private:
	// @brief 팀 ID
	FGenericTeamId mTeamId;
};
