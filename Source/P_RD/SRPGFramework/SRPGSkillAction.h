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
class UBoardActorModel;
enum class EForcedMovePresentationType : uint8;
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
	/** @brief 위치 개입 스킬의 플레이어 선택 착지 칸. Invalid면 후속 이동 없음. */
	UPROPERTY()
	FTileIndex mDisplacementDestination = FTileIndex::Invalid;
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

	// @brief 기존 강타/검기 슬롯을 선택 주사위 합만큼 적을 밀거나 당기는 개입으로 확장한다.
	bool TryStartDiceDisplacement(const FActiveSkillContext& Context, const UStaticSkillData* SkillData);
	/** @brief 적 역할별 공격에 명중 순간의 강제 이동을 붙인다. Spider=견인, Mushroom=밀치기. */
	bool TryStartEnemySignatureDisplacement(const FActiveSkillContext& Context);
	bool TryStartSkillDisplacement(const FActiveSkillContext& Context, const UStaticSkillData* SkillData);
	void StartDiceDisplacementStep(int32 StepIndex);
	void OnDiceDisplacementStepFinished();
	void ReportDiceDisplacementIfMoved();
	void BroadcastDiceDisplacementPath(EForcedMovePresentationType PresentationType) const;
	void FinishSkillAction();

private:
	UPROPERTY(Transient)
	TObjectPtr<UUnitModel> mDiceDisplacementTarget = nullptr;

	UPROPERTY(Transient)
	TArray<FTileIndex> mDiceDisplacementPath;

	UPROPERTY(Transient)
	TObjectPtr<UBoardActorModel> mDiceDisplacementBlocker = nullptr;

	int32 mDiceDisplacementStepIndex = 0;
	int32 mDiceDisplacementDiceValue = 0;
	FTileIndex mDiceDisplacementDestination = FTileIndex::Invalid;
	bool mDiceDisplacementIsPull = false;
	bool mDiceDisplacementIsThrow = false;
	bool mDiceDisplacementIsStagger = false;
	bool mDiceDisplacementIsSwap = false;
	bool mDiceDisplacementWasReported = false;
	bool mDiceDisplacementCollisionReported = false;
	bool mDiceDisplacementStarted = false;
	bool mDiceDisplacementFinished = false;
	bool mSkillPresentationFinished = false;
	bool mIsFixedIntentCast = false;
	bool mIsEnemySignatureDisplacement = false;
	FText mEnemySignatureSkillName;
};

