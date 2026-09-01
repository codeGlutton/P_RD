#pragma once

#include "RDMinimal.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardFlowTests.generated.h"

/** @brief 동적 멀티캐스트 델리게이트의 요청/성공 분리를 검증하는 테스트 수신기. */
UCLASS()
class URewardFlowTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	UFUNCTION()
	void HandleClaimConfirmed(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	int32 mRequestCount = 0;
	int32 mConfirmationCount = 0;
	ERewardClaimKind mLastRequestKind = ERewardClaimKind::Gold;
	ERewardClaimKind mLastConfirmationKind = ERewardClaimKind::Gold;
	int32 mLastRequestChoiceIndex = INDEX_NONE;
	int32 mLastConfirmationChoiceIndex = INDEX_NONE;
};
