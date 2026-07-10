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
 * 외부에서 미리 계산돼 넘어온 이동 포인트로 도달 범위를 정하고, 도달 가능 타일에서
 * 목적지를 골라 경로를 프리뷰한 뒤, 확정 시 이동 액션 생성 명령을 발행한다.
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
	void SetTargetTile(const FTileIndex& TileIndex);
	void ResetTargetTile();
	void BuildMove();

private:
	void SetBuildPhase(ESRPGMoveBuildPhase BuildPhase);

	/* 헬퍼 */
private:
	// @brief 턴 컨텍스트 → 전투 모델 → 타일 맵 모델을 꺼내온다 (없으면 checkf)
	UTileMapModel* GetTileMap() const;
	// @brief 현재 이동 포인트 기준으로 도달 범위를 재계산하고 강조 갱신
	void RefreshReachableTiles();

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

	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PathTileIndexes"))
	TArray<FTileIndex> mPathTileIndexes;
	UPROPERTY(Category = Build, EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetIndex"))
	FTileIndex mTargetIndex = FTileIndex::Invalid;
};
