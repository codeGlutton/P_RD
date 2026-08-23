#include "UI/Reward/RewardUIModel.h"

/** @brief UI의 Claim 입력을 보상 지급자가 구독하는 의도 이벤트로만 내보낸다. */
void URewardUIModel::RequestClaim()
{
	// 보상 지급/화면 전환은 구독한 게임플레이가 처리한다. UIModel은 "받기" 의도만 전달한다.
	OnRewardClaimed.Broadcast();
}

/** @brief 확정 보상 스냅샷을 저장하고 모든 바인딩 위젯에 다시 그리라고 알린다. */
void URewardUIModel::SetReward(const FRewardUI& Reward)
{
	// 전/후 값이 들어 있는 스냅샷을 보관해 WBP가 완성된 결과 문구를 즉시 다시 그릴 수 있게 한다.
	mReward = Reward;
	OnUIChanged.Broadcast();
}

/** @brief UI의 선택 입력을 보상 지급자가 구독하는 의도 이벤트로만 내보낸다. */
void URewardUIModel::RequestChooseReward(int32 ChoiceIndex)
{
	// 실제 지급/화면 전환은 구독한 게임플레이가 처리한다. UIModel은 "이 선택지를 골랐다" 신호만 전달한다.
	OnRewardChosen.Broadcast(ChoiceIndex);
}

void URewardUIModel::RequestClaimReward(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	OnRewardClaimRequested.Broadcast(ClaimKind, ChoiceIndex);

	if (ClaimKind == ERewardClaimKind::Choice)
	{
		OnRewardChosen.Broadcast(ChoiceIndex);
	}
}

bool URewardUIModel::RequestSelectReward(const FPrimaryAssetId RewardId)
{
	if (mAcquisitionPolicy != ERewardAcquisitionPolicy::SelectOne
		|| RewardId.IsValid() == false
		|| mSelectionOffer.mOptions.ContainsByPredicate(
			[&RewardId](const FRewardChoiceUI& Choice)
			{
				return Choice.mSourceAssetId == RewardId;
			}) == false)
	{
		return false;
	}

	OnRewardSelectionRequested.Broadcast(RewardId);
	return true;
}

bool URewardUIModel::RequestGrantBundle()
{
	if (mAcquisitionPolicy != ERewardAcquisitionPolicy::GrantAll)
	{
		return false;
	}

	OnRewardGrantBundleRequested.Broadcast();
	return true;
}

void URewardUIModel::ConfirmRewardClaim(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	OnRewardClaimConfirmed.Broadcast(ClaimKind, ChoiceIndex);
}

void URewardUIModel::ConfirmSelectedReward(const FPrimaryAssetId RewardId)
{
	if (RewardId.IsValid())
	{
		OnRewardSelectionConfirmed.Broadcast(RewardId);
	}
}

void URewardUIModel::ConfirmGrantBundle(const FRewardGrantBundleResultUI& Result)
{
	OnRewardGrantBundleConfirmed.Broadcast(Result);
}

/** @brief 룸 보상 항목 목록을 저장하고 보상 갱신 알림을 보낸다. */
void URewardUIModel::SetRewardChoices(const TArray<FRewardChoiceUI>& Choices)
{
	mChoices = Choices;
	mSelectionOffer.mOptions = Choices;
	mSelectionOffer.mSelectionCount = 1;
	mGrantBundle.mItems.Reset();
	mAcquisitionPolicy = Choices.IsEmpty()
		? ERewardAcquisitionPolicy::None : ERewardAcquisitionPolicy::SelectOne;
	OnChoicesChanged.Broadcast();
}

void URewardUIModel::SetSelectionOffer(const FRewardSelectionOfferUI& Offer)
{
	mSelectionOffer = Offer;
	mSelectionOffer.mSelectionCount = FMath::Max(1, mSelectionOffer.mSelectionCount);
	mGrantBundle.mItems.Reset();
	mChoices = mSelectionOffer.mOptions;
	mAcquisitionPolicy = mChoices.IsEmpty()
		? ERewardAcquisitionPolicy::None : ERewardAcquisitionPolicy::SelectOne;
	OnChoicesChanged.Broadcast();
}

void URewardUIModel::SetGrantBundle(const FRewardGrantBundleUI& Bundle)
{
	mGrantBundle = Bundle;
	mSelectionOffer.mOptions.Reset();
	mSelectionOffer.mSelectionCount = 1;
	mChoices = mGrantBundle.mItems;
	mAcquisitionPolicy = ERewardAcquisitionPolicy::GrantAll;
	OnChoicesChanged.Broadcast();
}
