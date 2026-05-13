/*****************************************************************//**
 * @file   Room.h
 * @brief  방 데이터 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Room.generated.h"

UENUM(BlueprintType)
enum class ERoomType : uint8
{
	None = 0 UMETA(Hidden),
	Treasure,
	Shop,
	Monster,
	EliteMonster,
	BossMonster,
};

/**
 * @brief  방 데이터
 */
USTRUCT(BlueprintType)
struct FRoom
{
	GENERATED_BODY()

public:
	FRoom() = default;
	virtual ~FRoom() = default;

public:
	virtual void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds);

public:
	UPROPERTY()
	ERoomType mType = ERoomType::None;
	UPROPERTY()
	int32 mRow = 0;
	UPROPERTY()
	int32 mColumn = 0;
	UPROPERTY()
	TArray<int32> mNextRoomColumns;

public:
	UPROPERTY()
	bool mIsSelected = false;
	UPROPERTY()
	FVector2D mPositionOffsetRate;

public:
	UPROPERTY()
	FPrimaryAssetId mStaticRoomSpawnDataId;
};

/**
 * @brief  보물 방 데이터
 */
USTRUCT(BlueprintType)
struct FTreasureRoom : public FRoom
{
	GENERATED_BODY()

public:
	FTreasureRoom();

public:
	void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) override;

public:
	UPROPERTY()
	FPrimaryAssetId mRewardEquipmentDataId;
};

/**
 * @brief  상점 방 데이터
 */
USTRUCT(BlueprintType)
struct FShopRoom : public FRoom
{
	GENERATED_BODY()

public:
	FShopRoom();

public:
	void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) override;

public:
	UPROPERTY()
	TArray<FPrimaryAssetId> mSaleSkillDataIds;
	UPROPERTY()
	TArray<FPrimaryAssetId> mSaleEquipmentDataIds;
};

/**
 * @brief  일반 몬스터 방 데이터
 */
USTRUCT(BlueprintType)
struct FMonsterRoom : public FRoom
{
	GENERATED_BODY()

public:
	FMonsterRoom();

public:
	UPROPERTY()
	int32 mRewardMoney;
	UPROPERTY()
	int32 mRewardExp;
};

/**
 * @brief  엘리트 몬스터 방 데이터
 */
USTRUCT(BlueprintType)
struct FEliteMonsterRoom : public FMonsterRoom
{
	GENERATED_BODY()

public:
	FEliteMonsterRoom();

public:
	void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) override;

public:
	UPROPERTY()
	FPrimaryAssetId mRewardEquipmentDataId;
};

/**
 * @brief  엘리트 몬스터 방 데이터
 */
USTRUCT(BlueprintType)
struct FBossMonsterRoom : public FMonsterRoom
{
	GENERATED_BODY()

public:
	FBossMonsterRoom();

public:
	void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) override;

public:
	UPROPERTY()
	FPrimaryAssetId mRewardDiceDataId;
};
