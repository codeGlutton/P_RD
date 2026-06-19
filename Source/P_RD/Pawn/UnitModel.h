/*****************************************************************//**
 * @file   UnitModel.h
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 모델 정의 파일
 * @author 모호재
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

#include "SRPGFramework/BoardActorModel.h"
#include "SRPGFramework/BoardCombatTarget.h"
#include "SRPGFramework/BoardSelectionTarget.h"
#include "GenericTeamAgentInterface.h"

#include "Unit.generated.h"

class UUnitModel;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnitDied, UUnitModel* /* Model */);

class UUnitAttributeSet;
class UPackageMap;
class UStaticUnitSpawnData;

class UGameplayAttributeComponent;
class USkillComponent;

/**
 * @brief  턴을 소유할 수 있는 베이스 폰 클래스 모델
 */
UCLASS(abstract)
class P_RD_API UUnitModel : public UBoardActorModel, public IGenericTeamAgentInterface, public IBoardCombatTarget, public IBoardSelectionTarget
{
	GENERATED_BODY()

public:
	UUnitModel();

	/* UBoardActorModel 상속 */
public:	
	void PostInitializeComponentModels() override;

public:
	ETileLayerFlag GetTileLayerFlags() const override;
	ETileLayerFlag GetBlockLayerFlags() const override;

	/* IGenericTeamAgentInterface 상속 */
public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

	/* IBoardCombatTarget 상속 */
public:
	UGameplayAttributeComponent* GetAttributeComponent() const override;
	FBoardCombatTargetSnapshotData* MakeSnapshotData() const override;

public:
	USkillComponent* GetSkillComponent() const;

public:
	void SetStaticSpawnData(const UStaticUnitSpawnData* StaticUnitSpawnData);

public:
	UUnitAttributeSet* GetUnitAttributeSet() const;

	FName GetUnitKeyName() const;
	const FText& GetUnitDisplayName() const;

	virtual int32 GetDifficulty() const PURE_VIRTUAL(AUnit::GetDifficulty, return 0;)
	virtual bool IsPlayerUnit() const PURE_VIRTUAL(AUnit::IsPlayerUnit, return false;)

public:
	FOnUnitDied OnUnitDied;

private:
	UPROPERTY(Category = GAS, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AbilitySystemComp"))
	TObjectPtr<UAbilitySystemComponent>	mAbilitySystemComp;

protected:
	UPROPERTY(Category = GAS, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;

private:
	UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "SkillComp"))
	TObjectPtr<USkillComponent>	mSkillComp;

protected:
	UPROPERTY(Category = Spawn, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "StaticUnitSpawnData"))
	TObjectPtr<const UStaticUnitSpawnData> mStaticUnitSpawnData;

private:
	// @brief 팀 ID
	FGenericTeamId mTeamId;
};
