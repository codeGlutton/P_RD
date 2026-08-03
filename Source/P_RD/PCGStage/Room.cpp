#include "PCGStage/Room.h"

#define LOCTEXT_NAMESPACE "Room"

void FRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	RoomId = mStaticRoomSpawnDataId;
}

FText FRoom::GetDisplayName() const
{
	switch (mType)
	{
	case ERoomType::Monster:      return LOCTEXT("RoomNameMonster", "일반");
	case ERoomType::EliteMonster: return LOCTEXT("RoomNameElite", "엘리트");
	case ERoomType::BossMonster:  return LOCTEXT("RoomNameBoss", "보스");
	case ERoomType::Shop:         return LOCTEXT("RoomNameShop", "상점");
	case ERoomType::Treasure:     return LOCTEXT("RoomNameTreasure", "보물");
	default:                      return FText::GetEmpty();
	}
}

FTreasureRoom::FTreasureRoom()
{
	mType = ERoomType::Treasure;
}

void FTreasureRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardEquipmentDataId);
}

FShopRoom::FShopRoom()
{
	mType = ERoomType::Shop;
}

void FShopRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	const int32 MaxJobCount = StaticCast<int32>(EUnitJobType::PlayerJobCount);
	for (int32 i = 0; i < MaxJobCount; ++i)
	{
		AdditionalAssetIds.Append(mSaleJobSkillDataItems[i].mSaleItemIds);
	}
	AdditionalAssetIds.Append(mSaleCommonSkillDataItems.mSaleItemIds);
	AdditionalAssetIds.Append(mSaleArtifactDataItems.mSaleItemIds);
}

FMonsterRoom::FMonsterRoom()
{
	mType = ERoomType::Monster;
}

FEliteMonsterRoom::FEliteMonsterRoom()
{
	mType = ERoomType::EliteMonster;
}

void FEliteMonsterRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardEquipmentDataId);
}

FBossMonsterRoom::FBossMonsterRoom()
{
	mType = ERoomType::BossMonster;
}

#undef LOCTEXT_NAMESPACE
