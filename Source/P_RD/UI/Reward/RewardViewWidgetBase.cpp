#include "UI/Reward/RewardViewWidgetBase.h"

#include "UI/Reward/RewardViewModel.h"

void URewardViewWidgetBase::BindViewModel(URewardViewModel* InViewModel)
{
	if (mViewModel == InViewModel)
	{
		return;
	}

	UnbindViewModel();
	mViewModel = InViewModel;

	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.AddDynamic(this, &URewardViewWidgetBase::HandleViewChanged);

		// 보상 데이터가 BindViewModel보다 먼저 들어온 경우도 있으므로 연결 직후 한 번 그려 초기 상태를 표시한다.
		OnRewardRefreshed();
	}
}

void URewardViewWidgetBase::Claim()
{
	if (mViewModel != nullptr)
	{
		mViewModel->RequestClaim();
	}
}

void URewardViewWidgetBase::UnbindViewModel()
{
	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.RemoveDynamic(this, &URewardViewWidgetBase::HandleViewChanged);
	}
	mViewModel = nullptr;
}

void URewardViewWidgetBase::HandleViewChanged()
{
	OnRewardRefreshed();
}

void URewardViewWidgetBase::NativeDestruct()
{
	UnbindViewModel();
	Super::NativeDestruct();
}
