/*****************************************************************//**
 * @file   SRPGUnit.h
 * @brief  SRPG에서 사용되는 베이스 폰 클래스 정의 파일
 * @author 모호재
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

#include "GameFramework/Pawn.h"
#include "SRPGFramework/TileActor.h"
#include "SRPGFramework/TileTargetable.h"
#include "SRPGFramework/TileSelectable.h"
#include "GenericTeamAgentInterface.h"

#include "Unit.generated.h"

class AUnit;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUnitDied, AUnit* /* Unit */);

class UUnitAttributeSet;
class UPackageMap;
class UStaticUnitSpawnData;

class USkillComponent;

/**
 * @brief  SRPG에서 사용되는 베이스 폰 클래스
 */
UCLASS(abstract)
class P_RD_API AUnit : public APawn, public IGenericTeamAgentInterface, public ITileActor, public ITileTargetable, public ITileSelectable
{
	GENERATED_BODY()

public:
	AUnit();

	/* APawn 상속 */
public:	
	void PostInitializeComponents() override;
	void OnConstruction(const FTransform& Transform) override;

	/* IGenericTeamAgentInterface 상속 */
public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

	/* ITileActor 상속 */
public:
	const FTileTransform& GetTileTransform() const override;
	ETileLayerFlag GetTileLayerFlags() const override;
	ETileLayerFlag GetBlockLayerFlags() const override;

	/* ITileTargetable 상속 */
public:
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	FTileTargetSnapshotTargetData* MakeSnapshotTargetData() const override;

protected:
	void SetTileTransform(const FTileTransform& Transform) override;

public:
	USkillComponent* GetSkillComponent() const;

public:
	virtual void OnBeginRoom();

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
	FTileTransform mTileTransform = FTileTransform::Invalid;
};
