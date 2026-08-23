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

	// 보상 아티팩트도 방 진입 전에 미리 로드 대상에 포함
	for (const FPrimaryAssetId& ArtifactId : GetEffectiveRewardArtifactDataIds())
	{
		if (ArtifactId.IsValid())
		{
			AdditionalAssetIds.Add(ArtifactId);
		}
	}
}

TArray<FPrimaryAssetId> FTreasureRoom::GetEffectiveRewardArtifactDataIds() const
{
	// mIsConfigured=true인 빈 배열은 의도적인 무보상이며 구형 필드를 부활시키지 않는다.
	return mRewardArtifactDataIds;
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
	for (const FMercenaryCandidate& Candidate : mSaleMercenaryDataCandidates.mCandidates)
	{
		AdditionalAssetIds.Add(Candidate.mSaleMercenaryId);
		AdditionalAssetIds.Append(Candidate.mOwingSkillIds);
	}
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

	for (const FPrimaryAssetId& ArtifactId : GetEffectiveRewardArtifactDataIds())
	{
		if (ArtifactId.IsValid())
		{
			AdditionalAssetIds.Add(ArtifactId);
		}
	}
}

TArray<FPrimaryAssetId> FEliteMonsterRoom::GetEffectiveRewardArtifactDataIds() const
{
	if (mIsConfigured || mRewardArtifactDataIds.IsEmpty() == false)
	{
		return mRewardArtifactDataIds;
	}

	TArray<FPrimaryAssetId> Result;
	if (mRewardArtifactDataId.IsValid())
	{
		Result.Add(mRewardArtifactDataId);
	}
	return Result;
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

	for (const FPrimaryAssetId& ArtifactId : GetEffectiveRewardArtifactDataIds())
	{
		if (ArtifactId.IsValid())
		{
			AdditionalAssetIds.Add(ArtifactId);
		}
	}
}

TArray<FPrimaryAssetId> FBossMonsterRoom::GetEffectiveRewardArtifactDataIds() const
{
	if (mIsConfigured || mRewardArtifactDataIds.IsEmpty() == false)
	{
		return mRewardArtifactDataIds;
	}

	TArray<FPrimaryAssetId> Result;
	if (mRewardArtifactDataId.IsValid())
	{
		Result.Add(mRewardArtifactDataId);
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
