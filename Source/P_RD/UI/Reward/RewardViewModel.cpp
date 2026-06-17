#include "UI/Reward/RewardViewModel.h"

void URewardViewModel::RequestClaim()
{
	// 보상 지급/화면 전환은 구독한 게임플레이가 처리한다. ViewModel은 "받기" 의도만 전달한다.
	OnRewardClaimed.Broadcast();
}

void URewardViewModel::SetReward(const FRewardView& Reward)
{
	// 전/후 값이 들어 있는 스냅샷을 보관해 WBP가 같은 데이터로 카운트업/막대 연출을 반복 재생할 수 있게 한다.
	mReward = Reward;
	OnViewChanged.Broadcast();
}
