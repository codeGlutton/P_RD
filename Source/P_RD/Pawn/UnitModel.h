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
class USkillComponentModel;
class UPassiveComponentModel;
class UEquipmentComponentModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, UUnitModel*, Model);

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
	void PostInitializeComponentModels() override;

public:
	void OnBeginRoom() override;
	void OnEndRoom() override;

public:
	/**
	 * @brief 자신의 턴 시작마다 실행될 함수
	 */
	virtual void OnBeginTurn();
	virtual void OnEndTurn();

	/* IGenericTeamAgentInterface 상속 */
public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

	/* IBoardCombatTarget 상속 */
public:
	UAttributeSetComponentModel* GetAttributeComponentModel() const override;

public:
	USkillComponentModel* GetSkillComponentModel() const;
	UPassiveComponentModel* GetPassiveComponentModel() const;
	UEquipmentComponentModel* GetEquipmentComponentModel() const;

public:
	virtual int32 GetDifficulty() const PURE_VIRTUAL(UUnitModel::GetDifficulty, return 0;)
	virtual bool IsPlayerUnitModel() const PURE_VIRTUAL(UUnitModel::IsPlayerUnit, return false;)

public:
	UPROPERTY(Category = Event, BlueprintAssignable)
	FOnUnitDied OnUnitDied;

private:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	TObjectPtr<UAttributeSetComponentModel> mAttributeCompModel;

	UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "SkillCompModel"))
	TObjectPtr<USkillComponentModel> mSkillCompModel;

	UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "PassiveCompModel"))
	TObjectPtr<UPassiveComponentModel> mPassiveCompModel;

	UPROPERTY(Category = Equipment, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "EquipmentCompModel"))
	TObjectPtr<UEquipmentComponentModel> mEquipmentCompModel;

private:
	// @brief 팀 ID
	FGenericTeamId mTeamId;
};
