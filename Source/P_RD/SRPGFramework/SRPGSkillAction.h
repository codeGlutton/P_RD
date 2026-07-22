/*****************************************************************//**
 * @file   SRPGSkillAction.h
 * @brief  스킬에 대한 SRPG 행동 객체 구현 헤더
 * @author 모호재
 * @date   2026-06-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGAction.h"
#include "SRPGFramework/SRPGCommand.h"
#include "SRPGSkillAction.generated.h"

class UTileMapModel;
struct FSRPGSkillAction;

USTRUCT()
struct FSRPGSkillCastCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGSkillCastCommand();

public:
	int32 mSkillIndex = 0;
	FTileIndex mTargetIndex = FTileIndex::Invalid;
};

/**
 * @brief  사용자 입력에 따른 정해진 SRPG 행동 객체
 */
UCLASS()
class USRPGSkillAction : public USRPGAction
{
	GENERATED_BODY()

protected:
	USRPGSkillAction();

protected:
	void OnBeginAction() override;
	void OnTickAction(float DeltaTime) override;
	void OnEndAction() override;

protected:
	ESRPGCommandResult HandleCommand(const TInstancedStruct<FSRPGCommand>& Command) override;

	/* 헬퍼 */
private:
	// @brief 턴 컨텍스트 → 전투 모델 → 타일 맵 모델을 꺼내온다
	UTileMapModel* GetTileMap() const;
};

