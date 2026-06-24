/*****************************************************************//**
 * @file   PassiveActivateContext.h
 * @brief  패시브 계산 입력 컨텍스트
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"

class UBoardActorModel;
struct FBoardCombatTargetSnapshotData;

/**
 * @brief 패시브 계산 입력 컨텍스트
 *
 * @details
 * 패시브가 효과를 계산(ActivatePassive)할 때 읽는 일회성 입력.
 * 패시브 내부 상태(mState)와 달리 호출마다 새로 채워짐.
 * 가변 수치는 스냅샷에서, 정체성(팀/직업)·위치는 모델 포인터에서 조회.
 */
struct FPassiveActivateContext
{
	// 패시브 소유자 (본인)
	TWeakObjectPtr<UBoardActorModel> mOwner;

	// 이번 계산 대상 (단수, 자기 대상이면 mOwner와 동일)
	TWeakObjectPtr<UBoardActorModel> mTarget;

	// 소유자 스냅샷 (base + 누적, 읽기 전용)
	const FBoardCombatTargetSnapshotData* mOwnerSnapshot = nullptr;

	// 대상 스냅샷 (base + 누적, 읽기 전용)
	const FBoardCombatTargetSnapshotData* mTargetSnapshot = nullptr;
};
