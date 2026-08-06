/*****************************************************************//**
 * @file   Room.h
 * @brief  방 데이터 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/RoomType.h"
#include "DataAsset/UnitSpawnData/UnitJobType.h"
#include "Room.generated.h"

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
	virtual void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const;

public:
	/*
	 * @brief 방 표시 이름 반환 
	 * @return 방 타입에 따른 이름
	 */
	FText GetDisplayName() const;

public:
	UPROPERTY(Category = Room, SaveGame, VisibleAnywhere, meta = (DisplayName = "Type"))
	ERoomType mType = ERoomType::None;
	UPROPERTY(Category = Room, SaveGame, VisibleAnywhere, meta = (DisplayName = "Row"))
	int32 mRow = 0;
	UPROPERTY(Category = Room, SaveGame, VisibleAnywhere, meta = (DisplayName = "Column"))
	int32 mColumn = 0;
	UPROPERTY(Category = Room, SaveGame, VisibleAnywhere, meta = (DisplayName = "NextRoomColumns"))
	TArray<int32> mNextRoomColumns;

public:
	UPROPERTY(Category = UI, SaveGame, VisibleAnywhere, meta = (DisplayName = "IsSelected"))
	bool mWasSelected = false;
	UPROPERTY(Category = UI, SaveGame, VisibleAnywhere, meta = (DisplayName = "PositionOffsetRate"))
	FVector2D mPositionOffsetRate = FVector2D::ZeroVector;

public:
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "PositionOffsetRate"))
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
	void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const override;

public:
	// 상자 개봉 시 지급될 골드
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "RewardMoney"))
	int32 mRewardMoney = 0;
	// 상자 개봉 시 지급될 아티팩트 목록
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "RewardArtifactDataIds"))
	TArray<FPrimaryAssetId> mRewardArtifactDataIds;
};

USTRUCT(BlueprintType)
struct FShopItemList
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "SaleCategory"))
	FName mSaleCategory;

	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "SaleItemIds"))
	TArray<FPrimaryAssetId> mSaleItemIds;
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
	void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const override;

public:
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (ArraySizeEnum = "EUnitJobType", DisplayName = "SaleJobSkillDataItems"))
	FShopItemList mSaleJobSkillDataItems[static_cast<uint8>(EUnitJobType::PlayerJobCount)];
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "SaleCommonSkillDataItems"))
	FShopItemList mSaleCommonSkillDataItems;
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "SaleEquipmentDataItems"))
	FShopItemList mSaleEquipmentDataItems;
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
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "RewardMoney"))
	int32 mRewardMoney = 0;
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "RewardExp"))
	int32 mRewardExp = 0;
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
	void CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const override;

public:
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "RewardEquipmentDataId"))
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
};
