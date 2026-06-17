/*****************************************************************//**
 * @file   SRPGEventLog.h
 * @brief  전투 결과 기록을 위한 구조체 헤더
 * @author 김준형, 모호재
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGEventLog.generated.h"

UENUM(BlueprintType)
enum class ESRPGTileOccupancyState : uint8
{
	None	UMETA(ToolTip = "변화 없음"),
	Move	UMETA(ToolTip = "이동"),
	Enter	UMETA(ToolTip = "입장 (또는 생성 : 액터 ID 필요)"),
	Exit	UMETA(ToolTip = "퇴장 (또는 사망, 제거 : 액터 ID 필요)"),
};

/**
 * @brief 상태를 변경시키는 이펙트 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGAttributeEffectEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return mEffectTag.IsValid() == true;
	}

public:
	// @brief 보드 액터의 변화된 속성
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTag"))
	FGameplayTag										mEffectTag;

	// @brief 보드 액터의 수치 변화량
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Magnitude"))
	float												mMagnitude = 0.f;
}

/**
 * @brief 타일을 이동시키는 이펙트 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGTileEffectEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return mOccupancyState != EOccupancyState::None;
	}

public:
	/**
	 * @brief 보드 액터의 점유 변화 상태
	 * @details
	 * None(변화 없음), Move(이동) Enter(생성), Exit(제거)
	 */
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "OccupancyState"))
	EOccupancyState										mOccupancyState = EOccupancyState::None;

public:
	// @brief 도달 지점
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "NextTileIndex"))
	FTileIndex											mNextTileIndex = FTileIndex::Invalid;
};

/**
 * @brief 액션 내 보드 액터에게 발생한 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGBoardActorEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return mTargetActorID != INDEX_NONE;
	}

public:
	// @brief 이벤트를 타겟이 된 액터 ID
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SourceUnitID"))
	int32												mTargetActorID = INDEX_NONE;

	// @brief 모션 처리 전 시작 지점
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StartTileIndex"))
	FTileIndex											mStartTileIndex = FTileIndex::Invalid;

public:
	// @brief 보드 액터의 타일 위치 변경 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TileEffectEventLogs"))
	TArray<FSRPGTileEffectEventLog>						mTileEffectEventLogs;

	// @brief 보드 액터의 속성 값 변경 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AttributeEffectEventLogs"))
	TMap<FGameplayTag, FSRPGAttributeEffectEventLog>	mAttributeEffectEventLogs;
};

/**
 * @brief 하나의 모션내에서 발생한 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGMotionEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return true;
	}

public:
	// @brief 한 모션 내에 각 액터마다의 변화 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BoardActorEventLogs"))
	TMap<int32, FSRPGBoardActorEventLog>				mBoardActorEventLogs;
};

/**
 * @brief 하나의 액션 내에서 발생한 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGActionEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return mSourceUnitID != INDEX_NONE && mSourceTileIndex != FTileIndex::Invalid;
	}

public:
	// @brief 이벤트를 발생 시킨 소스 유닛 ID
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SourceUnitID"))
	int32												mSourceUnitID = INDEX_NONE;

	// @brief 이벤트 초기에 위치한 소스 유닛의 타일 인덱스
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SourceTileIndex"))
	FTileIndex											mSourceTileIndex = FTileIndex::Invalid;

	// @brief 이벤트 초기에 지정한 타겟 타일 목록
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetTileIndexes"))
	TArray<FTileIndex>									mTargetTileIndexes;

public:
	// @brief 각 모션마다의 변화 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MotionEventLogs"))
	TArray<FSRPGMotionEventLog>							mMotionEventLogs;
};

