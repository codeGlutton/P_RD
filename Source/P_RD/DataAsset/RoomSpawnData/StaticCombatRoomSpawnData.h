/*****************************************************************//**
 * @file   StaticCombatRoomSpawnData.h
 * @brief  전투 방 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "StaticCombatRoomSpawnData.generated.h"

/**
 * @brief 맵 상에 유닛의 스폰 정보 구조체
 */
USTRUCT(BlueprintType)
struct FUnitSpawnData : public FActorSpawnData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Difficulty", ToolTip = "유닛 난이도"))
	int32 mDifficulty = 0;

public:
	UPROPERTY(Category = "Turn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TurnPriority", ToolTip = "턴 초기 배치 우선 순위"))
	int32 mTurnPriority = 0;
};

/**
 * @brief 타일 맵의 스폰 정보 구조체
 */
USTRUCT(BlueprintType)
struct FTileMapSpawnData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "Obstacle", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ObstacleSpawnDatas"))
	TArray<FActorSpawnData> mObstacleSpawnDatas;
};

/**
 * @brief  전투 방 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticCombatRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

public:
	void PostInitProperties() override;

public:
	/**
	 * @brief 현재 방이 등장할 수 있는 스테이지 레벨
	 * @details
	 * Primary Asset을 방 타입과 레벨 별로 분류해두었기 때문에, 해당 값은 Primary Asset Type에 영향을 줌
	 */
	UPROPERTY(Category = "Stage", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StageLevel"))
	EStageLevelType mStageLevel;

public:
	UPROPERTY(Category = "Unit", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PlayerTransform"))
	FTileTransform mPlayerTransform;
	
	UPROPERTY(Category = "Unit", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EnemySpawnDatas"))
	TArray<FUnitSpawnData> mEnemySpawnDatas;

public:
	UPROPERTY(Category = "TileMap", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TileMapSpawnData"))
	FTileMapSpawnData mTileMapSpawnData;
};

UCLASS()
class P_RD_API UStaticMonsterRoomSpawnData : public UStaticCombatRoomSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetMonsterRoomType(mStageLevel), GetFName());
	}
};

UCLASS()
class P_RD_API UStaticEliteMonsterRoomSpawnData : public UStaticCombatRoomSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetEliteMonsterRoomType(mStageLevel), GetFName());
	}
};

UCLASS()
class P_RD_API UStaticBossMonsterRoomSpawnData : public UStaticCombatRoomSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetBossMonsterRoomType(mStageLevel), GetFName());
	}
};

