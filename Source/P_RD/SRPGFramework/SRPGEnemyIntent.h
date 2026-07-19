/*****************************************************************//**
 * @file   SRPGEnemyIntent.h
 * @brief  플레이어 턴 전에 고정되는 적 행동 의도 스냅샷
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGEnemyIntent.generated.h"

class UEnemyUnitModel;

/** @brief 고정 의도가 실행되며 발생한 가장 최근의 핵심 결과. */
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

	UPROPERTY(BlueprintReadOnly)
	ESRPGEnemyIntentResult mResult = ESRPGEnemyIntentResult::Planned;

	UPROPERTY(BlueprintReadOnly)
	FText mResultText;

	UPROPERTY(BlueprintReadOnly)
	bool mWasDisplaced = false;
};
