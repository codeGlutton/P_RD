/*****************************************************************//**
 * @file   SRPGUnit.h
 * @brief  SRPG에서 사용되는 베이스 폰 클래스 정의 파일
 * @author 모호재
 * @date   2026-04-25
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
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
UCLASS()
class P_RD_API AUnit : public APawn, public IGenericTeamAgentInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUnit();

	/* Pawn 상속 */
public:	
	void PostInitializeComponents() override;
	void OnConstruction(const FTransform& transform) override;

	/* GenericTeamAgentInterface 상속 */
public:
	void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	FGenericTeamId GetGenericTeamId() const override;

	/* AbilitySystemInterface 상속 */
public:
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	virtual void OnBeginStage();
	virtual void OnEndStage();

public:
	virtual void SaveInfo(UUnitSaveGame* SaveGame) const;
	virtual void LoadInfo(const UUnitSaveGame* SaveGame);

public:
	/**
	 * 유닛의 현재 스탯 스냅샷을 찍어 타겟 정보로 반환하는 함수
	 */
	virtual FUnitSnapshotTargetData* MakeSnapshotTargetData() const;

public:
	const FText& GetDisplayName() const;
	FName GetRowKey() const;
	int32 GetDifficulty() const;

private:
	UPROPERTY(Category = GAS, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AbilitySystemComp"))
	TObjectPtr<UAbilitySystemComponent>	mAbilitySystemComp;
	UPROPERTY(Category = GAS, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UAttributeSet> mUnitAttributeSet;

protected:
	UPROPERTY(Category = Unit, EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Name"))
	FText mName;
	// @brief 초기 스텟에 반영되는 난이도 수치
	UPROPERTY(Category = Unit, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty;

private:
	FGenericTeamId mTeamId;
};
