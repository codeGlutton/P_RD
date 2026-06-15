#include "UI/Combat/CombatViewWidgetBase.h"

#include "UI/Combat/CombatViewModel.h"

void UCombatViewWidgetBase::BindViewModel(UCombatViewModel* InViewModel)
{
	if (mViewModel == InViewModel)
	{
		return;
	}

	UnbindViewModel();
	mViewModel = InViewModel;

	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.AddDynamic(this, &UCombatViewWidgetBase::HandleViewChanged);
		mViewModel->OnQueueNodeResolved.AddDynamic(this, &UCombatViewWidgetBase::HandleQueueNodeResolved);

		// 연결 직후 한 번 전체 갱신을 호출해 초기 상태를 그린다.
		OnViewRefreshed(ECombatViewDomain::All);
	}
}

void UCombatViewWidgetBase::UnbindViewModel()
{
	if (mViewModel != nullptr)
	{
		mViewModel->OnViewChanged.RemoveDynamic(this, &UCombatViewWidgetBase::HandleViewChanged);
		mViewModel->OnQueueNodeResolved.RemoveDynamic(this, &UCombatViewWidgetBase::HandleQueueNodeResolved);
	}
	mViewModel = nullptr;
}

void UCombatViewWidgetBase::HandleViewChanged(ECombatViewDomain Domain)
{
	OnViewRefreshed(Domain);
}

void UCombatViewWidgetBase::HandleQueueNodeResolved(FCombatQueueNode Node)
{
	OnQueueNodePlayed(Node);
}

void UCombatViewWidgetBase::NativeDestruct()
{
	UnbindViewModel();
	Super::NativeDestruct();
}
