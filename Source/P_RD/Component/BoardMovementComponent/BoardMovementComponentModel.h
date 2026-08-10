/*****************************************************************//**
 * @file   BoardMovementComponentModel.h
 * @brief  보드 액터 공용 이동 컴포넌트 모델 정의 헤더
 * @author 이문환
 * @date   2026-08-09
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "BoardMovementComponentModel.generated.h"

class UTileMapModel;

// @brief 이동 완료 통지 대리자 (MoveAlongPath 호출자가 요청 단위로 받음, 취소 시엔 호출되지 않음)
DECLARE_DELEGATE(FOnBoardMoveFinished);

/**
 * @brief  보드 액터 공용 이동 컴포넌트 모델
 *
 * @details
 * 경로를 받아서 한 칸씩 모델을 이동하고, 매 칸마다 이동 델리게이트와 연출 베리어 실행
 * 턴과 상관없이 모두 이 모델을 사용해서 이동
 * 이동 계산은 이 모델에서 처리하고, 뷰 컴포넌트는 델리게이트를 받아 표시만 함
 */
UCLASS()
class P_RD_API UBoardMovementComponentModel : public UComponentModel
{
	GENERATED_BODY()

	/* 이동 API */
public:
	/**
	 * @brief 확정 경로를 따라 스텝 이동 시작
	 * @param PathTileIndexes 시작→목표 경로 타일 목록 (양 끝 포함, 인덱스 0 = 현재 타일)
	 * @param OnFinished 이동 완료 통지
	 * @return 시작 성공 여부 (이동 중 재호출이거나 경로가 2칸 미만이면 false)
	 */
	bool MoveAlongPath(const TArray<FTileIndex>& PathTileIndexes, FOnBoardMoveFinished OnFinished = FOnBoardMoveFinished());

	// @brief 이동 진행 중 여부
	bool IsMoving() const;

	/**
	 * @brief 진행 중인 이동 취소
	 * @details 진행 중인 스텝의 연출이 끝나는 시점에 정지, 완료 통지 없음
	 *          점유는 이미 이번 스텝 타일로 옮겨졌으므로 되돌리지 않음
	 */
	void CancelMove();

	/* 파생 훅 */
protected:
	// @brief 스텝 시작 직전 훅 (유닛 파생의 AP 차감 등)
	virtual void OnStartStep(int32 StepIndex) {}

	/* 스텝 처리 */
private:
	// @brief StepIndex 칸으로 이동 시작 (모델 점유는 즉시, 도착 처리는 연출 후)
	void StartStep(int32 StepIndex);
	// @brief 현재 칸 도착 처리 (오버랩 통지)
	void CompleteStep();
	// @brief 이동연출베리어가 완료됐을 때 호출될 콜백 (다음 타일 진행/종료 판단)
	void OnStepPresentationFinished();

	/* 헬퍼 */
	// @brief 이동 상태 초기화 (완료/취소 공통 정리)
	void ResetMoveState();
	UTileMapModel* GetTileMap() const;

	/* 이동 상태 */
	// @brief 따라갈 경로 타일 목록 (인덱스 0은 시작 타일)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PathTileIndexes", AllowPrivateAccess = "true"))
	TArray<FTileIndex> mPathTileIndexes;

	// @brief 진행 중인 스텝 인덱스 (0은 시작 타일이라 1부터 시작, 이동 없음 = INDEX_NONE)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CurrentStepIndex", AllowPrivateAccess = "true"))
	int32 mCurrentStepIndex = INDEX_NONE;

	// @brief 취소 요청 여부 (현재 스텝 연출 종료 시점에 반영)
	bool mCancelRequested = false;

	// @brief 이번 이동의 완료 통지 대리자
	FOnBoardMoveFinished mOnFinished;
};
