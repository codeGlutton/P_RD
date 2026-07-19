/*****************************************************************//**
 * @file   SRPGEnemyIntent.h
 * @brief  플레이어 턴 전에 공개되고 플레이어 행동 뒤 갱신되는 적 행동 의도 스냅샷
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGEnemyIntent.generated.h"

class UEnemyUnitModel;

/** @brief 플레이어가 적의 위치를 바꾼 물리 개입 종류. */
UENUM(BlueprintType)
enum class ESRPGPlayerDisplacementType : uint8
{
	Push,
	Pull,
	Throw,
	Swap,
};

/** @brief 공개 의도가 실행되며 발생한 가장 최근의 핵심 결과. */
UENUM(BlueprintType)
enum class ESRPGEnemyIntentResult : uint8
{
	Planned,
	Executing,
	Completed,
	Missed,
	Collision,
	FriendlyFire,
	HitPlayer,
	HitObstacle,
	Cancelled,
};

/**
 * @brief UI와 결과 판정이 함께 읽는, 명령 객체와 분리된 적 행동 예고 데이터.
 * @details 명령 자체는 TurnContext에 보관하고 이 구조체에는 공개 가능한 계획 스냅샷만 둔다.
 */
USTRUCT(BlueprintType)
struct P_RD_API FSRPGEnemyIntent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 mTurnId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	int32 mExecutionOrder = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UEnemyUnitModel> mEnemy = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FText mSkillName;

	/** @brief 경로가 달라져도 유지하는 전술 목적(추격/거리 유지/사선 확보). */
	UPROPERTY(BlueprintReadOnly)
	FText mGoalText;

	/** @brief 최초 공개한 행동 정체성을 대응 계획에서도 유지하기 위한 스킬 슬롯. */
	UPROPERTY(BlueprintReadOnly)
	int32 mSkillIndex = INDEX_NONE;

	/** @brief 이번 적 턴에 사용할 수 있는 이동 예산. 대응 재계획에서도 증가하지 않는다. */
	UPROPERTY(BlueprintReadOnly)
	int32 mPlannedMoveRange = 0;

	UPROPERTY(BlueprintReadOnly)
	FTileIndex mPlannedOrigin = FTileIndex::Invalid;

	UPROPERTY(BlueprintReadOnly)
	FTileIndex mPlannedDestination = FTileIndex::Invalid;

	UPROPERTY(BlueprintReadOnly)
	FTileIndex mTargetTile = FTileIndex::Invalid;

	UPROPERTY(BlueprintReadOnly)
	TArray<FTileIndex> mPathTileIndexes;

	UPROPERTY(BlueprintReadOnly)
	TArray<FTileIndex> mEffectTileIndexes;

	/** @brief 마지막 재대응 직전 경로. 현재 경로와 함께 회색 잔상으로 한 번 비교 표시한다. */
	UPROPERTY(BlueprintReadOnly)
	TArray<FTileIndex> mPreviousPathTileIndexes;

	UPROPERTY(BlueprintReadOnly)
	FTileIndex mPreviousDestination = FTileIndex::Invalid;

	UPROPERTY(BlueprintReadOnly)
	ESRPGEnemyIntentResult mResult = ESRPGEnemyIntentResult::Planned;

	UPROPERTY(BlueprintReadOnly)
	FText mResultText;

	UPROPERTY(BlueprintReadOnly)
	bool mWasDisplaced = false;

	/** @brief 플레이어의 실제 이동/스킬이 끝난 뒤 이 계획을 다시 계산한 횟수. */
	UPROPERTY(BlueprintReadOnly)
	int32 mPlanRevision = 0;

	/** @brief 계획을 실제로 바꿀 때 지불한 누적 대응 이동력. 적은 영리하게 대응하지만 무한정 무료로 대응하지 않는다. */
	UPROPERTY(BlueprintReadOnly)
	int32 mResponseCostSpent = 0;

	/** @brief 플레이어 개입으로 마지막으로 밀려난 타일. 유닛이 제거된 뒤에도 조정 위치를 표시한다. */
	UPROPERTY(BlueprintReadOnly)
	FTileIndex mDisplacedToTile = FTileIndex::Invalid;

	/** @brief 첫 전투에서 밀어 AI의 즉시 재계산을 체험하기 쉬운 적으로 선택된 의도. */
	UPROPERTY(BlueprintReadOnly)
	bool mIsRecommendedInterventionTarget = false;
};
