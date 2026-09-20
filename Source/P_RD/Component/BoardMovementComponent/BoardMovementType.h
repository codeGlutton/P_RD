/*****************************************************************//**
 * @file   BoardMovementType.h
 * @brief  보드 이동 관련 공용 타입 정의 헤더
 * @author 이문환
 * @date   2026-08-12
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "BoardMovementType.generated.h"

/**
 * @brief 보드 이동 모드
 * @details 델리게이트(BoardActorModel.h)와 이동 컴포넌트 양쪽에서 사용하므로 독립 헤더로 분리
 */
UENUM(BlueprintType)
enum class EBoardMoveMode : uint8
{
	// @brief 자발적 일반 이동 (진행 방향 회전, 유닛은 AP 차감)
	Normal,
	// @brief 강제 밀려남 (바라보는 방향 유지, AP 미차감)
	Push,
	// @brief 강제 끌려옴 (밀려남과 같은 규칙이지만 연출/기록할 때 구분하기 위해 따로 추가)
	Pull,
};

/**
 * @brief 스스로 하는 이동인지 판정
 * @details 스스로 하는 이동은 상태이상(기절, 속박 등)으로 중단될 수 있음
 *          강제 이동(밀치기, 당기기 등)은 상태이상으로 중단되지 않음
 *          이동 모드가 추가되면 여기에 분류를 추가해야 됨
 */
inline bool IsSelfMove(EBoardMoveMode MoveMode)
{
	return MoveMode == EBoardMoveMode::Normal;
}
