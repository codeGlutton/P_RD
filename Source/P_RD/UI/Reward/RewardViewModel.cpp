#include "UI/Reward/RewardViewModel.h"

/** @brief UI의 Claim 입력을 보상 지급자가 구독하는 의도 이벤트로만 내보낸다. */
void URewardViewModel::RequestClaim()
{
	// 보상 지급/화면 전환은 구독한 게임플레이가 처리한다. ViewModel은 "받기" 의도만 전달한다.
	OnRewardClaimed.Broadcast();
}

/** @brief 확정 보상 스냅샷을 저장하고 모든 바인딩 위젯에 다시 그리라고 알린다. */
void URewardViewModel::SetReward(const FRewardView& Reward)
{
	// 전/후 값이 들어 있는 스냅샷을 보관해 WBP가 같은 데이터로 카운트업/막대 연출을 반복 재생할 수 있게 한다.
	mReward = Reward;
	OnViewChanged.Broadcast();
}
