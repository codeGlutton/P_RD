#include "UI/Reward/RewardRowWidgetBase.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void URewardRowWidgetBase::SetRewardRow(const FText& MainText, const FText& SubText, UTexture2D* IconTexture)
{
	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);
	mIsClaimed = false;
	mIsPressed = false;

	if (mRowIconFrame != nullptr)
	{
		mRowIconFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (mRewardIcon != nullptr)
	{
		if (IconTexture != nullptr)
		{
			mRewardIcon->SetBrushFromTexture(IconTexture, false);
			mRewardIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			mRewardIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const bool bHasSubText = SubText.IsEmpty() == false;
	if (mRewardSingleText != nullptr)
	{
		mRewardSingleText->SetText(MainText);
		mRewardSingleText->SetVisibility(bHasSubText ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	if (mRewardMainText != nullptr)
	{
		mRewardMainText->SetText(MainText);
		mRewardMainText->SetVisibility((bHasSubText || mRewardSingleText == nullptr) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (mRewardSubText != nullptr)
	{
		mRewardSubText->SetText(SubText);
		mRewardSubText->SetVisibility(bHasSubText ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void URewardRowWidgetBase::SetRewardIndex(int32 RewardRowIndex)
{
	mRewardRowIndex = RewardRowIndex;
}

void URewardRowWidgetBase::SetClaimed(bool bClaimed)
{
	mIsClaimed = bClaimed;
	mIsPressed = false;
	SetIsEnabled(bClaimed == false);
	SetVisibility(bClaimed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

FReply URewardRowWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (mIsClaimed == false && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		mIsPressed = true;
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply URewardRowWidgetBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (mIsPressed && mIsClaimed == false && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		mIsPressed = false;
		OnRewardRowClicked.Broadcast(mRewardRowIndex);
		return FReply::Handled();
	}

	mIsPressed = false;
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
