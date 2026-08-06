#include "PCGStage/Room.h"

#define LOCTEXT_NAMESPACE "Room"

void FRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	RoomId = mStaticRoomSpawnDataId;
}

FText FRoom::GetDisplayName() const
{
	return FText::GetEmpty();
}

FTreasureRoom::FTreasureRoom()
{
	mType = ERoomType::Treasure;
}

FText FTreasureRoom::GetDisplayName() const
{
	return LOCTEXT("RoomNameTreasure", "보물");
}

void FTreasureRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardArtifactDataId);
}

FShopRoom::FShopRoom()
{
	mType = ERoomType::Shop;

	const int32 PlayerUnitJobCount = StaticCast<int32>(EUnitJobType::PlayerJobCount);
	for (int32 PlayerUnitJobIndex = 0; PlayerUnitJobIndex < PlayerUnitJobCount; ++PlayerUnitJobIndex)
	{
		mSaleJobSkillDataItems[PlayerUnitJobIndex].mSaleCategory = EnumToText(StaticCast<EUnitJobType>(PlayerUnitJobIndex));
	}
	mSaleCommonSkillDataItems.mSaleCategory = EnumToText(EUnitJobType::Common);
	mSaleArtifactDataItems.mSaleCategory = LOCTEXT("Artifact", "아티팩트");
	mSaleMercenaryDataCandidates.mSaleCategory = LOCTEXT("Mercenary", "용병");
}

FText FShopRoom::GetDisplayName() const
{
	return LOCTEXT("RoomNameShop", "상점");
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

FText FMonsterRoom::GetDisplayName() const
{
	return LOCTEXT("RoomNameMonster", "일반");
}

FEliteMonsterRoom::FEliteMonsterRoom()
{
	mType = ERoomType::EliteMonster;
}

FText FEliteMonsterRoom::GetDisplayName() const
{
	return LOCTEXT("RoomNameElite", "엘리트");
}

void FEliteMonsterRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardArtifactDataId);
}

FBossMonsterRoom::FBossMonsterRoom()
{
	mType = ERoomType::BossMonster;
}

FText FBossMonsterRoom::GetDisplayName() const
{
	return LOCTEXT("RoomNameBoss", "보스");
}

void FBossMonsterRoom::CollectAssetIds(OUT FPrimaryAssetId& RoomId, OUT TArray<FPrimaryAssetId>& AdditionalAssetIds) const
{
	Super::CollectAssetIds(RoomId, AdditionalAssetIds);

	AdditionalAssetIds.Add(mRewardArtifactDataId);
}

#undef LOCTEXT_NAMESPACE
