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
class UStaticSkillData;
struct FActiveSkillContext;

USTRUCT()
struct FSRPGSkillCastCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGSkillCastCommand();

public:
	UPROPERTY()
	int32 mSkillIndex = 0;
	UPROPERTY()
	FTileIndex mTargetIndex = FTileIndex::Invalid;
	UPROPERTY()
	int32 mDiceSum = 0;

	// @brief 적 의도 계획 시 계산해 둔 효과 타일. 실행 시 재계산하지 않는다.
	UPROPERTY()
	TArray<FTileIndex> mFixedEffectTileIndexes;

	UPROPERTY()
	bool mUseFixedIntent = false;

	// @brief 고정 적 공격만 팀 필터를 풀어 다른 적/장애물도 실제 피격되게 한다.
	UPROPERTY()
	bool mAllowFriendlyFire = false;
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

	// @brief 기존 Smash 스킬을 선택 주사위 합만큼 실제 보드 유닛을 미는 개입으로 확장한다.
	bool TryStartDicePush(const FActiveSkillContext& Context, const UStaticSkillData* SkillData);
	void StartDicePushStep(int32 StepIndex);
	void OnDicePushStepFinished();
	void ReportDicePushIfMoved();
	void FinishSkillAction();

private:
	UPROPERTY(Transient)
	TObjectPtr<UUnitModel> mDicePushTarget = nullptr;

	UPROPERTY(Transient)
	TArray<FTileIndex> mDicePushPath;

	int32 mDicePushStepIndex = 0;
	int32 mDicePushDiceValue = 0;
	bool mDicePushWasReported = false;
	bool mIsFixedIntentCast = false;
};

