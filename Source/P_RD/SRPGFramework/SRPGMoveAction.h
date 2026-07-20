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
class UBoardActorModel;
class UEnemyUnitModel;

// @brief 확정된 이동 경로를 실어 이동 액션 생성을 요청하는 명령
USTRUCT()
struct FSRPGMoveCommand : public FSRPGCommand
{
	GENERATED_BODY()

public:
	FSRPGMoveCommand();

public:
	// @brief 시작→목표 순서의 경로 타일 목록(양 끝 포함)
	UPROPERTY()
	TArray<FTileIndex> mPathTileIndexes;

	// @brief true면 경로 이탈/점유 변화가 생겨도 재탐색하지 않고 예정 경로를 시도한다.
	UPROPERTY()
	bool mUseFixedIntent = false;

	/** @brief 플레이어가 직접 그린 직선 돌진인지 여부. 충돌 시 체급별 강제 이동을 실행한다. */
	UPROPERTY()
	bool mIsWarriorCharge = false;

	/** @brief 중간 타일을 검사하지 않고 도착 타일로 뛰어드는 기사 도약. */
	UPROPERTY()
	bool mIsWarriorLeap = false;

	/** @brief Slime의 이동 자체를 탄성 돌진으로 실행한다. 경로의 유닛을 한 칸 밀고 충돌 지점까지 진입한다. */
	UPROPERTY()
	bool mIsElasticCharge = false;

	/** @brief 돌진 행동에 정의된 거리와 밀어내기 위력. */
	UPROPERTY()
	int32 mActionPower = 0;

	/** @brief false면 스킬 주사위를 이미 소비했으므로 기존 이동력은 차감하지 않는다. */
	UPROPERTY()
	bool mConsumeMovementPoints = true;
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
	// @brief 고정 의도 이동의 출발점/다음 칸이 여전히 유효한지 검사한다.
	bool ValidateFixedIntentStep(int32 StepIndex);
	// @brief 현재 칸 도착 처리 (오버랩 통지)
	void CompleteStep();
	// @brief 이동연출베리어가 완료됐을 때 호출될 콜백 (그 다음 타일로 이동한다든지)
	void OnStepPresentationFinished();
	// @brief 돌진의 다음 칸을 막은 적을 체급에 따라 밀고, 끝나면 플레이어 이동을 재개/정지한다.
	bool TryStartWarriorChargeImpact(int32 StepIndex);
	void StartWarriorChargePushStep(int32 StepIndex);
	void OnWarriorChargePushStepFinished();
	void FinishWarriorChargeImpact();
	bool TryStartElasticChargeImpact(int32 StepIndex);
	void StartElasticChargePush();
	void OnElasticChargePushFinished();
	UBoardActorModel* FindBlockingActor(const FTileIndex& TileIndex, const UBoardActorModel* MovingActor) const;

	/* 헬퍼 */
private:
	UTileMapModel* GetTileMap() const;

protected:
	// @brief 따라갈 경로 타일 목록 (인덱스 0은 시작 타일)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "PathTileIndexes"))
	TArray<FTileIndex> mPathTileIndexes;

	// @brief 적 라운드 시작에 공개된 경로라면 현재 상황에 맞춰 우회하지 않는다.
	bool mUseFixedIntent = false;
	bool mIsWarriorCharge = false;
	bool mIsWarriorLeap = false;
	bool mIsElasticCharge = false;
	bool mConsumeMovementPoints = true;
	int32 mActionPower = 0;

	// @brief 진행 중인 스텝 인덱스 (mPathTileIndexes 기준, 0은 시작 타일이라 1부터 시작)
	UPROPERTY(Category = Move, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "CurrentStepIndex"))
	int32 mCurrentStepIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<UEnemyUnitModel> mWarriorChargeTarget = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UBoardActorModel> mWarriorChargeBlocker = nullptr;
	TArray<FTileIndex> mWarriorChargePushPath;
	FTileIndex mWarriorChargeFromTile = FTileIndex::Invalid;
	int32 mWarriorChargePushStepIndex = 0;
	int32 mWarriorChargeResumePlayerStep = INDEX_NONE;
	bool mWarriorChargeContinueAfterImpact = false;
	bool mWarriorChargeStopAfterPlayerStep = false;

	UPROPERTY(Transient)
	TObjectPtr<UUnitModel> mElasticChargeTarget = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UBoardActorModel> mElasticChargeBlocker = nullptr;
	FTileIndex mElasticChargeFromTile = FTileIndex::Invalid;
	FTileIndex mElasticChargeDestination = FTileIndex::Invalid;
	int32 mElasticChargeResumeStep = INDEX_NONE;
	bool mElasticChargeStopAfterStep = false;
};
