#include "UI/CombatResultOverlayWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "CombatResultOverlayWidget"

UCombatResultOverlayWidget::UCombatResultOverlayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = 60;
	mRemoveFromParentOnClose = true;
}

void UCombatResultOverlayWidget::ShowVictoryReward(const FRewardUI& Reward, FSimpleDelegate ConfirmCallback)
{
	mMode = ECombatResultOverlayMode::VictoryReward;
	mReward = Reward;
	mTitleCallback = MoveTemp(ConfirmCallback);
	RefreshWidget();
}

void UCombatResultOverlayWidget::ShowDefeatResult(
	const FCombatResultUI& Result,
	FSimpleDelegate TitleCallback)
{
	mMode = ECombatResultOverlayMode::DefeatContinue;
	mReward = FRewardUI();
	mCombatResult = Result;
	mTitleCallback = MoveTemp(TitleCallback);
	BindButtons();
	RefreshWidget();
}

void UCombatResultOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogRD, Display, TEXT("Combat defeat WBP construct: title=%s"),
		mTitleButton != nullptr ? TEXT("bound") : TEXT("missing"));

	BindButtons();

	RefreshWidget();
}

void UCombatResultOverlayWidget::NativeDestruct()
{
	if (mTitleButton != nullptr)
	{
		mTitleButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
	}
	Super::NativeDestruct();
}

void UCombatResultOverlayWidget::BindButtons()
{
	if (mTitleButton != nullptr)
	{
		mTitleButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
		mTitleButton->OnClicked.AddUniqueDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
	}
}

void UCombatResultOverlayWidget::HandleTitleClicked()
{
	UE_LOG(LogRD, Display, TEXT("Combat defeat title button clicked."));
	FSimpleDelegate Callback = MoveTemp(mTitleCallback);
	mTitleCallback.Unbind();
	Callback.ExecuteIfBound();
}

void UCombatResultOverlayWidget::RefreshWidget()
{
	if (mMode != ECombatResultOverlayMode::DefeatContinue)
	{
		return;
	}

	// The board supplies the artwork; native text keeps live values and localization.
	if (mLocationText != nullptr)
	{
		mLocationText->SetText(mCombatResult.mLocationName.IsEmpty()
			? LOCTEXT("UnknownLocation", "현재 전투 지역") : mCombatResult.mLocationName);
	}
	if (mRoundText != nullptr)
	{
		mRoundText->SetText(FText::AsNumber(FMath::Max(1, mCombatResult.mRound)));
	}
	if (mEnemyText != nullptr)
	{
		mEnemyText->SetText(FText::AsNumber(FMath::Max(0, mCombatResult.mDefeatedMonsterCount)));
	}
}

#undef LOCTEXT_NAMESPACE
