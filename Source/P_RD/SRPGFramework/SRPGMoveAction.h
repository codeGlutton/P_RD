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
 * 빌드 액션이 확정해 실어 보낸 경로(FSRPGMoveCommand)를 받아 이동 컴포넌트 모델에 전달
 * 실제 이동은 컴포넌트 모델이 처리하고, 액션은 완료를 기다렸다가 턴 시스템에 보고
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

protected:
	// @brief 따라갈 경로 타일 목록 (인덱스 0은 시작 타일)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PathTileIndexes"))
	TArray<FTileIndex> mPathTileIndexes;
};
