/*****************************************************************//**
 * @file   GASTargetFunctionLibrary.h
 * @brief  GAS에 Target Data에 대한 확장 함수 라이브러리 헤더
 * @author 모호재
 * @date   2026-04-29
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"

class AUnit;
struct FUnitSnapshotTargetData;

/**
 * @brief  GAS에 Target Data에 대한 확장 함수 라이브러리
 */
class P_RD_API UGASTargetFunctionLibrary
{
	static FGameplayAbilityTargetDataHandle MakeSnapshotTargetDataHandle(const AUnit* Source);
	static FGameplayAbilityTargetDataHandle MakeSnapshotTargetDataHandle(const AUnit* Source, const AUnit* Target);

	static const FUnitSnapshotTargetData* GetSnapshotTargetData(const FGameplayAbilityTargetDataHandle& Handle, int32 Index);

	template<typename UnitSnapshotType>
	static const UnitSnapshotType* GetSnapshotTargetData(const FGameplayAbilityTargetDataHandle& Handle, int32 Index)
	{
		return StaticCast<UnitSnapshotType*>(GetSnapshotTargetData(Handle, Index));
	}
};
