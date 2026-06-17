#include "UI/Reward/RewardViewModel.h"

void URewardViewModel::RequestClaim()
{
	OnRewardClaimed.Broadcast();
}

void URewardViewModel::SetReward(const FRewardView& Reward)
{
	mReward = Reward;
	OnViewChanged.Broadcast();
}
