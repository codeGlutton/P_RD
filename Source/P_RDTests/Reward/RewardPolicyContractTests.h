#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "RewardPolicyContractTests.generated.h"

class URewardPolicyTestListener;

UCLASS()
class P_RDTESTS_API URewardPolicyTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleSelectionRequested(FPrimaryAssetId RewardId);

	UFUNCTION()
	void HandleGrantBundleRequested();

	int32 SelectionRequestCount = 0;
	int32 GrantBundleRequestCount = 0;
	FPrimaryAssetId LastSelectedRewardId;
};
