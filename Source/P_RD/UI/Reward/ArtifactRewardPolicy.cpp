#include "UI/Reward/ArtifactRewardPolicy.h"

namespace ArtifactRewardPolicy
{
	bool Contains(const TArray<FPrimaryAssetId>& CandidateIds,
		const FPrimaryAssetId& RequestedId)
	{
		return RequestedId.IsValid()
			&& CandidateIds.Contains(RequestedId);
	}

	bool TrySelectOne(const TArray<FPrimaryAssetId>& CandidateIds,
		const FPrimaryAssetId& RequestedId,
		OUT FPrimaryAssetId& SelectedId)
	{
		SelectedId = FPrimaryAssetId();
		if (Contains(CandidateIds, RequestedId) == false)
		{
			return false;
		}

		SelectedId = RequestedId;
		return true;
	}

	FRewardGrantBundleResultUI GrantAll(
		const TArray<FPrimaryAssetId>& ArtifactIds,
		TFunctionRef<bool(const FPrimaryAssetId&)> Grant)
	{
		FRewardGrantBundleResultUI Result;
		Result.mGrantedItemIds.Reserve(ArtifactIds.Num());
		Result.mFailedItemIds.Reserve(ArtifactIds.Num());

		for (const FPrimaryAssetId& ArtifactId : ArtifactIds)
		{
			if (Grant(ArtifactId))
			{
				Result.mGrantedItemIds.Add(ArtifactId);
			}
			else
			{
				Result.mFailedItemIds.Add(ArtifactId);
			}
		}

		return Result;
	}
}
