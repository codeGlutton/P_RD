/*****************************************************************//**
 * @file   UnitMovementComponentModel.h
 * @brief  유닛 전용 이동 컴포넌트 모델 정의 헤더
 * @author 이문환
 * @date   2026-08-10
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"
#include "UnitMovementComponentModel.generated.h"

/**
 * @brief  유닛 전용 이동 컴포넌트 모델
 *
 * @details
 * 공용 이동에 유닛 전용 규칙을 얹음 - 스텝마다 AP 차감, 상태이상 기반 이동 가능 질의
 */
UCLASS()
class P_RD_API UUnitMovementComponentModel : public UBoardMovementComponentModel
{
	GENERATED_BODY()

public:
	// @brief 스스로 움직일 수 있는 상태인지 판정. 이동을 막는 상태이상 태그를 매번 확인
	bool CanSelfMove() const override;

protected:
	// @brief 스텝 시작마다 AP 1 차감
	void OnStartStep(int32 StepIndex, EBoardMoveMode MoveMode) override;
};
