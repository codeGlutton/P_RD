#include "UI/Reward/RewardRowWidgetBase.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

URewardRowWidgetBase::URewardRowWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// RDUserWidget 공용 버튼 클릭음과 같은 음원(SVN uasset)을 하드레퍼런스로 프리로드 → 쿡 보장(#300 컨벤션).
	static ConstructorHelpers::FObjectFinder<USoundBase> RowClickSoundFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Audio/UISFX/SFX_UI_Click_Scratch003.SFX_UI_Click_Scratch003"));
	if (RowClickSoundFinder.Succeeded())
	{
		mRowClickSound = RowClickSoundFinder.Object;
	}
}

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
		// 행은 UButton이 아니라 스타일 사운드가 없으므로 여기서 공용 클릭음을 직접 재생한다.
		if (mRowClickSound != nullptr)
		{
			UGameplayStatics::PlaySound2D(this, mRowClickSound);
		}
		OnRewardRowClicked.Broadcast(mRewardRowIndex);
		return FReply::Handled();
	}

	mIsPressed = false;
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}
