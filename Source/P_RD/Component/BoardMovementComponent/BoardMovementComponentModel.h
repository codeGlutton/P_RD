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
#include "Component/BoardMovementComponent/BoardMovementType.h"
#include "BoardMovementComponentModel.generated.h"

class UTileMapModel;

// @brief 이동 완료 통지 대리자 (MoveAlongPath/PushAlongPath 호출자가 요청 단위로 받음, 취소 시엔 호출되지 않음)
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
	 * @brief 확정 경로를 따라 스텝 이동 시작 (일반 이동 모드 — 진행 방향 회전, 유닛은 AP 차감)
	 * @param PathTileIndexes 시작→목표 경로 타일 목록 (양 끝 포함, 인덱스 0 = 현재 타일)
	 * @param OnFinished 이동 완료 통지
	 * @return 시작 성공 여부 (이동 중 재호출이거나 경로가 2칸 미만이면 false)
	 */
	bool MoveAlongPath(const TArray<FTileIndex>& PathTileIndexes, FOnBoardMoveFinished OnFinished = FOnBoardMoveFinished());

	/**
	 * @brief 확정 경로를 따라 밀치기(강제 이동) 시작
	 * @details 일반 이동과 달리 바라보는 방향 유지, AP 미차감. 그 외 스텝 루프/베리어/오버랩은 공유
	 * @param PathTileIndexes 시작→목표 경로 (양 끝 포함, 인덱스 0 = 현재 타일). 벽이나 장애물에 막히는 건 미리 계산해서 경로로 줘야한다
	 * @param OnFinished 이동 완료 통지
	 * @return 시작 성공 여부 (이동 중 재호출이거나 경로가 2칸 미만이면 false)
	 */
	bool PushAlongPath(const TArray<FTileIndex>& PathTileIndexes, FOnBoardMoveFinished OnFinished = FOnBoardMoveFinished());

	/**
	 * @brief 이동 루프 안(도착 오버랩 통지 중)에서 함정이 보류 밀치기 경로를 등록
	 * @details 직접 이동을 시작하는 재진입 대신 등록만 하고, 현재 스텝 마무리 지점에서 루프가 소비.
	 *          정지 상태의 대상을 미는 경우는 이 함수가 아니라 PushAlongPath 사용
	 * @param TrapTileIndex 발동한 함정의 타일 (연쇄 기록 키)
	 * @param PushPathTileIndexes 밀치기 경로 (인덱스 0 = 피격자의 현재 타일)
	 * @return 등록 성공 여부. 이동 중이 아니거나, 이번 연쇄에서 이미 발동한 함정이거나, 경로가 2칸 미만이면 false
	 */
	bool TryRegisterPendingPush(const FTileIndex& TrapTileIndex, const TArray<FTileIndex>& PushPathTileIndexes);

	// @brief 이동 진행 중 여부
	bool IsMoving() const;

	// @brief 현재 이동 모드 (이동 중이 아니면 마지막 모드)
	EBoardMoveMode GetMoveMode() const;

	/**
	 * @brief 진행 중인 이동 취소
	 * @details 진행 중인 스텝의 연출이 끝나는 시점에 정지, 완료 통지 없음
	 *          점유는 이미 이번 스텝 타일로 옮겨졌으므로 되돌리지 않음
	 */
	void CancelMove();

	/* 파생 훅 */
protected:
	// @brief 스텝 시작 직전 훅 (유닛 파생의 AP 차감 등). MoveMode로 일반이동/밀치기이동 구분
	virtual void OnStartStep(int32 StepIndex, EBoardMoveMode MoveMode) {}

	// @brief 타일맵 획득 (테스트용 파생 컴포넌트가 오버라이드 할 수 있게 가상함수로 선언)
	virtual UTileMapModel* GetTileMap() const;

	/* 스텝 처리 */
private:
	// @brief 공용 이동 시작 (MoveAlongPath/PushAlongPath의 실제 구현)
	bool StartPathInternal(const TArray<FTileIndex>& PathTileIndexes, EBoardMoveMode MoveMode, FOnBoardMoveFinished OnFinished);
	// @brief 전체 경로를 월드 좌표로 변환해서 뷰에 통지 (이동 시작/경로 교체 공용)
	void BroadcastStartMovePath();
	// @brief StepIndex 칸으로 이동 시작 (모델 점유는 즉시, 도착 처리는 연출 후)
	void StartStep(int32 StepIndex);
	// @brief 현재 칸 도착 처리 (오버랩 통지)
	void CompleteStep();
	// @brief 이동연출베리어가 완료됐을 때 호출될 콜백 (다음 타일 진행/종료 판단)
	void OnStepPresentationFinished();

	/* 헬퍼 */
	// @brief 이동 상태 초기화 (완료/취소 공통 정리)
	void ResetMoveState();

	/* 이동 상태 */
	// @brief 따라갈 경로 타일 목록 (인덱스 0은 시작 타일)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PathTileIndexes", AllowPrivateAccess = "true"))
	TArray<FTileIndex> mPathTileIndexes;

	// @brief 진행 중인 스텝 인덱스 (0은 시작 타일이라 1부터 시작, 이동 없음 = INDEX_NONE)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CurrentStepIndex", AllowPrivateAccess = "true"))
	int32 mCurrentStepIndex = INDEX_NONE;

	// @brief 현재 이동 모드 (일반/밀치기)
	EBoardMoveMode mMoveMode = EBoardMoveMode::Normal;

	// @brief 이번 연쇄에서 발동한 함정 타일 기록 (같은 함정 재발동 금지 → 연쇄 종료 보장)
	TSet<FTileIndex> mChainedTrapTiles;

	// @brief 보류 밀치기 경로 (함정이 등록, 스텝 마무리 지점에서 루프가 소비)
	TArray<FTileIndex> mPendingPushPath;

	// @brief 이번 요청에서 채택한 밀치기 경로 수 (버그성 무한 연쇄 checkf 백스톱)
	int32 mAdoptedPushCount = 0;

	// @brief 연쇄 깊이 상한 (정상 연쇄는 이보다 짧음 — 초과 = 연쇄 기록 버그)
	static constexpr int32 MaxPushChainDepth = 9;

	// @brief 취소 요청 여부 (현재 스텝 연출 종료 시점에 반영)
	bool mCancelRequested = false;

	// @brief 이번 이동의 완료 통지 대리자
	FOnBoardMoveFinished mOnFinished;
};
