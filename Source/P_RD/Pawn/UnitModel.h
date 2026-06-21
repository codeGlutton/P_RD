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
#include "Actor/BoardActor/BoardSelectionTarget.h"
#include "GenericTeamAgentInterface.h"

#include "UnitModel.generated.h"

class UUnitModel;

class UUnitAttributeSet;
class UStaticUnitSpawnData;

class UGameplayAttributeComponentModel;
class USkillComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, UUnitModel*, Model);

/**
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 모델
 */
UCLASS(abstract)
class P_RD_API UUnitModel : public UBoardActorModel, public IBoardCombatTarget, public IBoardSelectionTarget
{
	GENERATED_BODY()

public:
	UUnitModel();

	/* UBoardActorModel 상속 */
public:	
	void PostInitializeComponentModels() override;

	/* IGenericTeamAgentInterface 상속 */
public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

	/* IBoardCombatTarget 상속 */
public:
	UGameplayAttributeComponentModel* GetAttributeComponentModel() const;
	FBoardCombatTargetSnapshotData* MakeSnapshotData() const override;

public:
	// USkillComponent* GetSkillComponent() const;

public:
	virtual int32 GetDifficulty() const PURE_VIRTUAL(UUnitModel::GetDifficulty, return 0;)
	virtual bool IsPlayerUnitModel() const PURE_VIRTUAL(UUnitModel::IsPlayerUnit, return false;)

public:
	UPROPERTY(Category = Event, BlueprintAssignable)
	FOnUnitDied OnUnitDied;

private:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	TObjectPtr<UGameplayAttributeComponentModel> mAttributeCompModel;

	// TODO : 모델로 바꿔야됨
	// UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "SkillComp"))
	// TObjectPtr<USkillComponent>	mSkillComp;

private:
	// @brief 팀 ID
	FGenericTeamId mTeamId;
};
