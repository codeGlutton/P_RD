/*****************************************************************//**
 * @file   SRPGMoveBuildAction.h
 * @brief  이동 생성 액션 객체 구현 헤더
 * @author 이문환
 * @date   2026-06-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGMoveBuildAction.generated.h"

class USRPGMoveBuildAction;
class UTileMapModel;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangeMoveBuildPhase, const USRPGMoveBuildAction* /*Action*/, ESRPGMoveBuildPhase /*Phase*/);


// @brief 이동 빌드 진입 명령 (이동 액션 생성을 요청)
USTRUCT(BlueprintType)
struct FSRPGMoveSelectCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGMoveSelectCommand();

public:
	FOnChangeMoveBuildPhase OnChangeMoveBuildPhase;
};

/**
 * @brief  이동 생성 액션 객체
 *
 * @details
 * 스킬 생성 액션(USRPGSkillBuildAction)을 본떠 만든 이동 전용 빌드 액션이다.
 * 외부에서 미리 계산돼 넘어온 이동 포인트로 도달 범위를 정하고, 도달 가능 타일을
 * 누를 때마다 경유지까지의 부분경로가 추가된다. 마지막 경유지를 한 번 더 누르면
 * 부분경로들을 이어붙인 경로로 이동 액션 생성 명령을 발행한다. 이전 경유지를
 * 다시 눌러도 새 경유지로 추가돼 왔다갔다 하는 경로를 만들 수 있고(마커 순번은
 * 마지막 방문이 덮어씀), 출발 타일로 되돌아오는 경로도 허용한다. 범위 밖을
 * 누르면 마지막 경유지를 취소한다.
 * 이동 포인트는 한 번에 다 쓰지 않아도 되며, 이동 후 남은 포인트로 이동 빌드를 다시
 * 호출해 총 이동 거리를 여러 번에 나눠 사용할 수 있다.
 * 스킬과 달리 사용할 스킬은 이동 스킬 하나로 고정이라 스킬 인덱스를 두지 않는다.
 */
UCLASS()
class USRPGMoveBuildAction : public USRPGAction
{
	GENERATED_BODY()

public:
	USRPGMoveBuildAction();
	/** @brief 현재 선택한 전체 경로가 소비할 AP. 이동 한 칸당 1을 사용한다. */
	int32 GetPlannedMoveCost() const;

	/* FSRPGAction 상속 */
protected:
	void OnBeginAction() override;
	void OnEndAction() override;

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

protected:
	ESRPGCommandResult HandleWorldTraceCommand(const TInstancedStruct<FSRPGCommand>& Command);

	/* 빌드 로직 처리 */
private:
	void EnterMoveBuild();
	void ResetMoveBuild();
	// @brief 마지막 경유지에서 지정 타일까지 부분경로를 추가 (경유지 추가)
	void AddWaypoint(const FTileIndex& TileIndex);
	// @brief 마지막 경유지 취소 (부분경로 하나 제거, 남은 부분경로가 없으면 목적지 선택 단계로 복귀)
	void RemoveLastWaypoint();
	void BuildMove();

private:
	void SetBuildPhase(ESRPGMoveBuildPhase BuildPhase);

	/* 헬퍼 */
private:
	// @brief 턴 컨텍스트 → 전투 모델 → 타일 맵 모델을 꺼내온다 (없으면 checkf)
	UTileMapModel* GetTileMap() const;
	// @brief 마지막 경유지 기준, 남은 이동 포인트로 도달 범위를 재계산하고 강조 갱신
	void RefreshReachableTiles();
	// @brief 마지막 경유지 좌표 (부분경로가 없으면 유닛 현재 위치)
	FTileIndex GetLastWaypoint() const;
	// @brief 남은 이동 포인트 (보유 포인트 - 부분경로들이 사용할 포인트 합)
	int32 GetRemainMovePoint() const;
	// @brief 경로들의 집합을 하나의 경로로 이어붙여서 전체 경로를 표시
	void RefreshPathPreview();

protected:
	FOnChangeMoveBuildPhase OnChangeMoveBuildPhase;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MoveBuildPhase"))
	ESRPGMoveBuildPhase mMoveBuildPhase = ESRPGMoveBuildPhase::None;

protected:
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ReachableTileIndexes"))
	TArray<FTileIndex> mReachableTileIndexes;

	// @brief 이번 빌드에서 사용할 이동 포인트(외부에서 계산된 도달 거리, 분할 이동 시 남은 포인트)
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MovePoint"))
	int32 mMovePoint = 0;

	// @brief 부분경로 리스트 (부분경로 = FindPath 결과, 양 끝 포함. 부분경로의 끝 타일 = 경유지, 마지막 부분경로의 끝 타일 = 도착 후보)
	// @note 중첩 TArray는 UPROPERTY 노출 불가. FTileIndex는 UObject 참조가 없어 GC 관리도 불필요
	TArray<TArray<FTileIndex>> mPathSegments;
};
