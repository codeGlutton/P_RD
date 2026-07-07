/*****************************************************************//**
 * @file   EventLog.h
 * @brief  전투 결과 기록을 위한 구조체 헤더
 * @author 김준형, 모호재
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "EventLog.generated.h"

UENUM(BlueprintType)
enum class ESRPGTileOccupancyState : uint8
{
	None	UMETA(ToolTip = "변화 없음"),
	Move	UMETA(ToolTip = "이동"),
	Enter	UMETA(ToolTip = "입장 (또는 생성 : 액터 ID 필요)"),
	Exit	UMETA(ToolTip = "퇴장 (또는 사망, 제거 : 액터 ID 필요)"),
};

/**
 * @brief 속성값을 변경시키는 이펙트 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGAttributeEffectEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return mEffectAttribute.IsValid() == true;
	}

public:
	bool operator==(const FSRPGAttributeEffectEventLog& Other) const
	{
		return mEffectAttribute == Other.mEffectAttribute;
	}
	bool operator!=(const FSRPGAttributeEffectEventLog& Other) const
	{
		return mEffectAttribute != Other.mEffectAttribute;
	}

	friend uint32 GetTypeHash(const FSRPGAttributeEffectEventLog& Log)
	{
		return GetTypeHash(Log.mEffectAttribute);
	}

public:
	// @brief 보드 액터의 변화된 속성
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectAttribute"))
	FTacticalAttribute										mEffectAttribute;

	// @brief 보드 액터의 수치 변화량
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Magnitude"))
	float													mMagnitude = 0.f;
};

/**
 * @brief 태그 상태를 변경시키는 이펙트 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGTagEffectEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return mEffectTag.IsValid() == true;
	}

public:
	bool operator==(const FSRPGTagEffectEventLog& Other) const
	{
		return mEffectTag == Other.mEffectTag;
	}
	bool operator!=(const FSRPGTagEffectEventLog& Other) const
	{
		return mEffectTag != Other.mEffectTag;
	}

	friend uint32 GetTypeHash(const FSRPGTagEffectEventLog& Log)
	{
		return GetTypeHash(Log.mEffectTag);
	}

public:
	// @brief 보드 액터의 변화된 태그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTag"))
	FGameplayTag											mEffectTag;

	// @brief 보드 액터의 태그 개수 변화량
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Count"))
	int32													mCount = 0;
};

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
		return mOccupancyState != ESRPGTileOccupancyState::None;
	}

public:
	/**
	 * @brief 보드 액터의 점유 변화 상태
	 * @details
	 * None(변화 없음), Move(이동) Enter(생성), Exit(제거)
	 */
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "OccupancyState"))
	ESRPGTileOccupancyState									mOccupancyState = ESRPGTileOccupancyState::None;

public:
	// @brief 시작 지점
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PreTileIndex"))
	FTileIndex												mPreTileIndex = FTileIndex::Invalid;

	// @brief 도달 지점
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "NextTileIndex"))
	FTileIndex												mNextTileIndex = FTileIndex::Invalid;
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
		return mTargetActorID != INDEX_NONE && mBoardActorModelClass != nullptr;
	}

public:
	// @brief 이벤트를 타겟이 된 액터 ID
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SourceUnitID"))
	int32													mTargetActorID = INDEX_NONE;

	// @brief 이벤트를 타겟이 된 보드 액터 모델 클래스
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BoardActorModelClass", MustImplement = "/Script/P_RD.BoardActorModel"))
	TSubclassOf<UObject>									mBoardActorModelClass = nullptr;

public:
	// @brief 보드 액터의 타일 위치 변경 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TileEffectEventLogs"))
	TArray<FSRPGTileEffectEventLog>							mTileEffectEventLogs;

	// @brief 보드 액터의 태그 갯수 변경 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TagEffectEventLogs"))
	TSet<FSRPGTagEffectEventLog>							mTagEffectEventLogs;

	// @brief 보드 액터의 속성 값 변경 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AttributeEffectEventLogs"))
	TSet<FSRPGAttributeEffectEventLog>						mAttributeEffectEventLogs;
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
	TMap<int32, FSRPGBoardActorEventLog>					mBoardActorEventLogs;

	// @brief 한 모션 내에 각 액터마다의 변화 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SpawnedBoardActorPositions"))
	TMap<int32, FTileIndex>									mSpawnedBoardActorPositions;
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
		return mSourceTileIndex != FTileIndex::Invalid;
	}

public:
	// @brief 액션 초기에 위치한 소스 유닛의 타일 인덱스
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SourceTileIndex"))
	FTileIndex												mSourceTileIndex = FTileIndex::Invalid;

public:
	// @brief 각 모션마다의 변화 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "MotionEventLogs"))
	TArray<FSRPGMotionEventLog>								mMotionEventLogs;
};

/**
 * @brief 하나의 턴 내에서 발생한 이벤트 로그
 */
USTRUCT(BlueprintType)
struct FSRPGTurnEventLog
{
	GENERATED_BODY()

public:
	bool IsValid() const
	{
		return mSourceUnitID != INDEX_NONE && mUnitActorModelClass != nullptr;
	}

public:
	// @brief 이벤트를 발생 시킨 소스 유닛 ID
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SourceUnitID"))
	int32													mSourceUnitID = INDEX_NONE;

	// @brief 이벤트를 발생 시킨 액터 모델 클래스
	UPROPERTY(Category = "Params", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "BoardActorModelClass", MustImplement = "/Script/P_RD.BoardActorModel"))
	TSubclassOf<UObject>									mUnitActorModelClass = nullptr;

public:
	// @brief 각 액션마다의 변화 로그
	UPROPERTY(Category = "Result", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ActionEventLogs"))
	TArray<FSRPGActionEventLog>								mActionEventLogs;
};
