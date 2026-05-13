#include "PCGStage/Room.h"

void FRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds)
{
	RoomId = mStaticRoomSpawnDataId;
}

FTreasureRoom::FTreasureRoom()
{
	mType = ERoomType::Treasure;
}

void FTreasureRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds)
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardEquipmentDataId);
}

FShopRoom::FShopRoom()
{
	mType = ERoomType::Shop;
}

void FShopRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds)
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Append(mSaleSkillDataIds);
	AdditionalAssetIds.Append(mSaleEquipmentDataIds);
}

FMonsterRoom::FMonsterRoom()
{
	mType = ERoomType::Monster;
}

FEliteMonsterRoom::FEliteMonsterRoom()
{
	mType = ERoomType::EliteMonster;
}

void FEliteMonsterRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds)
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardEquipmentDataId);
}

FBossMonsterRoom::FBossMonsterRoom()
{
	mType = ERoomType::BossMonster;
}

void FBossMonsterRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds)
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardDiceDataId);
}
