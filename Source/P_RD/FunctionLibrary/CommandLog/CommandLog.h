/*****************************************************************//**
 * @file   CommandLog.h
 * @brief  전투 결과 기록을 위한 구조체 헤더
 * @author 김준형
 * @date   2026-06-11
 *********************************************************************/
#pragma once

#include "GAS/GASMinimal.h"
#include "UObject/Object.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/TileActor.h"
#include "CommandLog.generated.h"

UENUM(BlueprintType)
enum class EOccupancyState : uint8
{
	None,			// 변화 없음
	Enter,			// 입장 (또는 생성 : 액터 ID 필요)
	Exit,			// 퇴장 (또는 사망, 제거 : 액터 ID 필요)
	Replace,		// 대체 (생성 종류 명시 필요)
	Spawn			// 스폰 (생성 종류 명시 필요)
};

UENUM(BlueprintType)
enum class EEffectEventTiming : uint8
{
	SkillDefaultCommit	UMETA(ToolTip = "스킬 적용 시점"),
	EndAttacking		UMETA(ToolTip = "공격 종료 패시브 발동 시점"),
	EndHitting			UMETA(ToolTip = "피격 종료 패시브 발동 시점"),
	EndMotion			UMETA(ToolTip = "모션 종료 패시브 발동 시점"),
	EndUseSkill			UMETA(ToolTip = "스킬 종료 패시브 발동 시점")
};

// @brief 장판의 변화
USTRUCT(BlueprintType)
struct FOverlayEventLog
{
	GENERATED_BODY()

public:
	// 장판
	int32 mTargetUnitID;

	// 장판의 점유 변화 : None(변화 없음), 입장(생성), 퇴장(제거), 변경
	EOccupancyState mOccupancyState;

	// 생성 대상의 종류

	// 데이터가 채워져 있는지 확인하는 함수
	bool IsValid() const
	{
		return mTargetUnitID != 0; // 또는 식별 가능한 기본값 조건
	}

};

// @brief 유닛의 변화
USTRUCT(BlueprintType)
struct FUnitEventLog
{
	GENERATED_BODY()

public:
	// 유닛
	int32 mTargetUnitID;

	// 유닛의 점유 변화 : None(변화 없음),입장(생성), 퇴장(제거), 변경
	EOccupancyState mOccupancyState;

	// 생성 대상의 종류

	// 유닛의 상태 변화
	// 이벤트 : 수치
	FGameplayTag mGameplayTag;
	float mValue;

	// 데이터가 채워져 있는지 확인하는 함수
	bool IsValid() const
	{
		return mTargetUnitID != 0; // 또는 식별 가능한 기본값 조건
	}
};

// @brief 장애물 변화
USTRUCT(BlueprintType)
struct FObstacleEventLog
{
	GENERATED_BODY()

public:
	// 장애물
	int32 mTargetUnitID;

	// 장애물의 점유 변화 : None(변화 없음),입장(생성), 퇴장(제거), 변경
	EOccupancyState mOccupancyState;

	// 생성 대상의 종류

	// 데이터가 채워져 있는지 확인하는 함수
	bool IsValid() const
	{
		return mTargetUnitID != 0; // 또는 식별 가능한 기본값 조건
	}
};

// @brief 점유의 상태의 변화
USTRUCT(BlueprintType)
struct FEventLog
{
	GENERATED_BODY()

public:
	FOverlayEventLog	mOverlayEventLog;	// 장판 상태의 변화
	FUnitEventLog		mUnitEventLog;		// 유닛 상태의 변화
	FObstacleEventLog	mObstacleEventLog;	// 장애물 상태의 변화
};

// @brief 타일의 변화
USTRUCT(BlueprintType)
struct FTileLog
{
	GENERATED_BODY()

public:
	EEffectEventTiming mEventTimig;
	TMap<int32, FEventLog> mEventLog;
};

// @brief 전체 변화
USTRUCT(BlueprintType)
struct FCommandLog
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Log")
	TArray<FTileLog> mTileLog;
};


#pragma region TileMapClone

USTRUCT(BlueprintType)
struct FTileActorCloneData
{
	GENERATED_BODY()

public:

	int32 mActorType;

	int32 mID;
	float mDiceCount;
	float mDiceDots;
	float HP;

	// ...
	TMap<FString, float> mPassiveDynamic;

	// 패시브 로직
};

USTRUCT(BlueprintType)
struct FTileCloneData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<int32> mActorIDs; // 타일에 존재하는 대상의 아이디
};

// 
// @brief TEST용 SnapShot
USTRUCT(BlueprintType)
struct FTileMapCloneData
{
	GENERATED_BODY()

public:
	TArray<FTileCloneData>						mTiles;					// 타일
	TMap<int32, FTileActorCloneData>	mTileActorDatas;		// 타일맵에 존재하는 액터들
};

#pragma endregion


// @brief 어떤 대상의 시뮬레이터가 필요한 것인가?
UENUM(BlueprintType)
enum class ECommandLogRequestType : uint8
{
	Skill,					// 스킬 발동 시 시뮬레이터
	Passive,				// 패시브 발동 시 시뮬레이터
	OverlayTileActor,		// 장판 발동 시 시뮬레이터
};

// 
// @brief 커맨드 로그 함수 실행 시 필요한 정보 구조체
USTRUCT(BlueprintType)
struct FCommandLogFunctionContext
{
	GENERATED_BODY()

public:
	int32 mSourceActorID;					// 소스 액터
	TArray<FTileIndex> mTargetTiles;			// 타겟 타일

	ECommandLogRequestType mRequestType;

	class UStaticSkillData* mSkillData;				// 스킬 효과
	//TArray<class UPassive> mPassiveData;				// 패시브 효과
	//TArray<class TOverlayTileActor> mOverlayTileActor;// 장판 효과
};