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
	/**
	 * Gameplay Event에 유닛의 스냅샷을 담아 전달하기 위해 Target Data를 만들어 인코딩하는 함수
	 * @param Source Ability를 실행한 유닛
	 * @return Target Data 핸들 객체
	 */
	static FGameplayAbilityTargetDataHandle MakeSnapshotTargetDataHandle(const AUnit* Source);
	/**
	 * Gameplay Event에 유닛의 스냅샷을 담아 전달하기 위해 Target Data를 만들어 인코딩하는 함수
	 * @param Source Ability를 실행한 유닛
	 * @param Target Ability의 로직 대상이 될 유닛
	 * @return Target Data 핸들 객체
	 */
	static FGameplayAbilityTargetDataHandle MakeSnapshotTargetDataHandle(const AUnit* Source, const AUnit* Target);
	/**
	 * Gameplay Event로 전달받은 유닛의 스냅샷 Target Data를 살펴보기 위해 디코딩하는 함수
	 * @param Handle Target Data 핸들 객체
	 * @param Index 핸들 내 몇번째 Target Data를 디코딩할지
	 * @return 디코딩한 Target Data
	 */
	static const FUnitSnapshotTargetData* GetSnapshotTargetData(const FGameplayAbilityTargetDataHandle& Handle, int32 Index);

	template<typename UnitSnapshotType>
	static const UnitSnapshotType* GetSnapshotTargetData(const FGameplayAbilityTargetDataHandle& Handle, int32 Index)
	{
		return StaticCast<UnitSnapshotType*>(GetSnapshotTargetData(Handle, Index));
	}
};
