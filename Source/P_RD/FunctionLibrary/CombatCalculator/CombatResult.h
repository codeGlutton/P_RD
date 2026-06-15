/*****************************************************************//**
 * @file   CombatResult.h
 * @brief  전투 결과 기록을 위한 구조체 헤더
 * @author 김준형
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "GAS/GASMinimal.h"
#include "UObject/Object.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/TileActor.h"
#include "CombatResult.generated.h"

// @brief 유닛 적용 결과
USTRUCT(BlueprintType)
struct FUnitCommitResult
{
	GENERATED_BODY()

public:

	// 임시 체력, 보호막
	// 유닛에게 이걸 적용했다.
	TMap<FGameplayTag, float> mEffect;

	TMap<FName, float> mPassive;
};

// @brief 타일 적용 결과
USTRUCT(BlueprintType)
struct FTileActorCommitResult
{
	GENERATED_BODY()

public:

	TScriptInterface<ITileActor> mTileActor;

	// 유닛에게 이걸 적용했다.
	FUnitCommitResult mUnitCommitResult;

	// 타일에 이것을 적용했다. 소환 등
	// ???
};

// @brief 이펙트 적용 결과
USTRUCT(BlueprintType)
struct FEffectCommitResult
{
	GENERATED_BODY()

public:
	TArray<FTileActorCommitResult> mTileCommitResult;

};

// @brief 스킬 적용 결과
USTRUCT(BlueprintType)
struct FSkillCommitResult
{
	GENERATED_BODY()

public:
	TArray<FEffectCommitResult> mEffectCommitResult;
};
