/*****************************************************************//**
 * @file   StaticCombatRoomSpawnData.h
 * @brief  전투 방 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/StaticRoomSpawnData.h"
#include "StaticCombatRoomSpawnData.generated.h"

/**
 * @brief 맵 상에 유닛의 스폰 정보 구조체
 */
USTRUCT(BlueprintType)
struct FUnitSpawnData : public FActorSpawnData
{
	GENERATED_BODY()

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
UCLASS()
class P_RD_API UStaticCombatRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		const FString TypeString = FString(TEXT("CombatRoom")) + FString::FromInt(mLevel);
		return FPrimaryAssetId(*TypeString, GetFName());
	}

public:
	UPROPERTY(Category = "Unit", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PlayerTransform"))
	FTileTransform mPlayerTransform;
	
	UPROPERTY(Category = "Unit", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EnemySpawnDatas"))
	TArray<FUnitSpawnData> mEnemySpawnDatas;

public:
	UPROPERTY(Category = "TileMap", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TileMapSpawnData"))
	FTileMapSpawnData mTileMapSpawnData;
};
