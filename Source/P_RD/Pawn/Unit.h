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

class UUnitAttributeSet;
class UUnitSaveGame;
class UPackageMap;

USTRUCT(Blueprintable)
struct FUnitSnapshotTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

public:
	UScriptStruct* GetScriptStruct() const override;
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& OutSuccess);

public:
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mMaxHP;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mHP;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mSkillPoint;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mDamagePoint;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mDefensePoint;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mMovementPoint;

	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	uint8 mBuffState;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	uint8 mDebuffState;
};

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

	/* GenericTeamAgentInterface 상속 */
public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

	/* ITileActor 상속 */
public:
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	virtual void OnBeginStage();
	virtual void OnEndStage();

public:
	/**
	 * 유닛의 현재 스탯 스냅샷을 찍어 타겟 정보로 반환하는 함수
	 */
	virtual FUnitSnapshotTargetData* MakeSnapshotTargetData() const;

public:
	UUnitAttributeSet* GetUnitAttributeSet() const;

	FName GetUnitName() const;
	const FText& GetUnitDisplayName() const;

	virtual int32 GetDifficulty() const PURE_VIRTUAL(FUnitSnapshotTargetData::GetDifficulty, return 0;)

private:
	UPROPERTY(Category = GAS, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AbilitySystemComp"))
	TObjectPtr<UAbilitySystemComponent>	mAbilitySystemComp;
	UPROPERTY(Category = GAS, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;

protected:
	// @brief UI 표기 이름
	UPROPERTY(Category = Unit, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "UnitDisplayName"))
	FText mUnitDisplayName;

private:
	// @brief 팀 ID
	FGenericTeamId mTeamId;
};
