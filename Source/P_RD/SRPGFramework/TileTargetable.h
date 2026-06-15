/*****************************************************************//**
 * @file   TileTargetable.h
 * @brief  SRPG 타일에 타격 가능한 객체 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "UObject/Interface.h"
#include "TileTargetable.generated.h"

USTRUCT(Blueprintable)
struct FTileTargetSnapshotTargetData : public FGameplayAbilityTargetData
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

UINTERFACE(MinimalAPI)
class UTileTargetable : public UAbilitySystemInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 타일에 타격 가능한 객체
 */
class P_RD_API ITileTargetable : public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/**
	 * 현재 타격 가능한 여부를 반환하는 함수
	 * @return 타격 가능 여부 
	 */
	virtual bool IsTargetable() const;
	bool IsDead() const;

public:
	/**
	 * 유닛의 현재 스탯 스냅샷을 찍어 타겟 정보로 반환하는 함수
	 */
	virtual FTileTargetSnapshotTargetData* MakeSnapshotTargetData() const = 0;
};
