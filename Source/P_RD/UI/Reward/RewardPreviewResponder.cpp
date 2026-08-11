#include "UI/Reward/RewardPreviewResponder.h"

#include "UI/Reward/RewardUIModel.h"

void URewardPreviewResponder::Bind(URewardUIModel* UIModel)
{
	if (UIModel == nullptr)
	{
		return;
	}

	if (mUIModel != UIModel)
	{
		Unbind();
	}
	mUIModel = UIModel;
	mUIModel->OnRewardClaimRequested.AddUniqueDynamic(
		this, &URewardPreviewResponder::HandleClaimRequested);
	mUIModel->OnRewardClaimed.AddUniqueDynamic(
		this, &URewardPreviewResponder::HandleClaimed);
}

void URewardPreviewResponder::SetOnPreviewClosed(FSimpleDelegate InCallback)
{
	mOnPreviewClosed = MoveTemp(InCallback);
}

void URewardPreviewResponder::HandleClaimed()
{
	FSimpleDelegate ClosedCallback = MoveTemp(mOnPreviewClosed);
	Unbind();
	if (ClosedCallback.IsBound())
	{
		ClosedCallback.Execute();
	}
}

void URewardPreviewResponder::HandleClaimRequested(
	ERewardClaimKind ClaimKind,
	int32 ChoiceIndex)
{
	if (mUIModel != nullptr)
	{
		mUIModel->ConfirmRewardClaim(ClaimKind, ChoiceIndex);
	}
}

void URewardPreviewResponder::Unbind()
{
	if (mUIModel == nullptr)
	{
		return;
	}

	mUIModel->OnRewardClaimRequested.RemoveDynamic(
		this, &URewardPreviewResponder::HandleClaimRequested);
	mUIModel->OnRewardClaimed.RemoveDynamic(
		this, &URewardPreviewResponder::HandleClaimed);
	mUIModel = nullptr;
}
