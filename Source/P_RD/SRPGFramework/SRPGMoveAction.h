/*****************************************************************//**
 * @file   SRPGMoveAction.h
 * @brief  이동에 대한 SRPG 행동 객체 구현 헤더
 * @author 이문환
 * @date   2026-06-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGMoveAction.generated.h"

class UTileMapModel;

// @brief 확정된 이동 경로를 실어 이동 액션 생성을 요청하는 명령
USTRUCT()
struct FSRPGMoveCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGMoveCommand();

public:
	// @brief 시작→목표 순서의 경로 타일 목록(양 끝 포함)
	TArray<FTileIndex> mPathTileIndexes;
};

/**
 * @brief  확정된 경로를 따라 유닛을 이동시키는 SRPG 행동 객체
 *
 * @details
 * 스킬 행동 액션(USRPGSkillAction)을 본떠 만든 이동 전용 실행 액션이다.
 * 빌드 액션이 확정해 실어 보낸 경로(FSRPGMoveCommand)를 받아, 경로를 한 칸씩
 * StartActorMovement/CompleteActorMovement로 밟아 이동시킨다.
 */
UCLASS()
class USRPGMoveAction : public USRPGAction
{
	GENERATED_BODY()

protected:
	USRPGMoveAction();

protected:
	void OnBeginAction() override;
	void OnEndAction() override;

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

	/* 이동 처리 */
private:
	// @brief StepIndex 칸으로 이동 시작 (모델 점유는 즉시, 도착 처리는 연출 후)
	void StartStep(int32 StepIndex);
	// @brief 현재 칸 도착 처리 (오버랩 통지)
	void CompleteStep();

	/* 헬퍼 */
private:
	UTileMapModel* GetTileMap() const;

protected:
	// @brief 따라갈 경로 타일 목록 (인덱스 0은 시작 타일)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PathTileIndexes"))
	TArray<FTileIndex> mPathTileIndexes;
};
