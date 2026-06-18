#include "UI/Reward/MockRewardDriver.h"

#include "UI/Reward/RewardViewModel.h"

/** @brief 개발용 고정 보상 스냅샷을 ViewModel에 주입해 WBP 연동만 검증한다. */
void UMockRewardDriver::Start(URewardViewModel* ViewModel)
{
	if (ViewModel == nullptr)
	{
		return;
	}
	mViewModel = ViewModel;

	// Mock은 실제 보상 지급자가 아니다. Claim 입력이 UI에서 ViewModel을 타고 올라오는지만 확인한다.
	mViewModel->OnRewardClaimed.AddDynamic(this, &UMockRewardDriver::HandleClaimed);

	// 가짜 보상 fixture: 돈 50, 경험치 30 (레벨 3, 40→70 / 최대 100). 밸런스 데이터로 쓰면 안 된다.
	FRewardView Reward;
	Reward.mGoldGained = 50;
	Reward.mGoldBalance = 170;
	Reward.mExpGained = 30;
	Reward.mLevelBefore = 3;
	Reward.mLevelAfter = 3;
	Reward.mExpBefore = 40.f;
	Reward.mExpAfter = 70.f;
	Reward.mMaxExp = 100.f;
	mViewModel->SetReward(Reward);
}

/** @brief Claim 이벤트가 ViewModel을 통해 되돌아왔는지만 확인한다. */
void UMockRewardDriver::HandleClaimed()
{
	UE_LOG(LogRD, Display, TEXT("MockRewardDriver: reward claimed"));
}
