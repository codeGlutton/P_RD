#include "UI/CombatResultOverlayWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "CombatResultOverlayWidget"

UCombatResultOverlayWidget::UCombatResultOverlayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = -15;
	mRemoveFromParentOnClose = true;
}

void UCombatResultOverlayWidget::ShowVictoryReward(const FRewardUI& Reward, FSimpleDelegate ConfirmCallback)
{
	mMode = ECombatResultOverlayMode::VictoryReward;
	mReward = Reward;
	mConfirmCallback = MoveTemp(ConfirmCallback);
	RefreshSlate();
}

void UCombatResultOverlayWidget::ShowDefeatContinue(FSimpleDelegate ContinueCallback)
{
	mMode = ECombatResultOverlayMode::DefeatContinue;
	mReward = FRewardUI();
	mConfirmCallback = MoveTemp(ContinueCallback);
	RefreshSlate();
}

TSharedRef<SWidget> UCombatResultOverlayWidget::RebuildWidget()
{
	const FLinearColor PanelColor(0.018f, 0.022f, 0.028f, 0.88f);
	const FLinearColor ButtonColor(0.65f, 0.49f, 0.21f, 0.96f);
	const FLinearColor TextColor(0.96f, 0.94f, 0.84f, 1.0f);
	const FLinearColor MutedTextColor(0.78f, 0.82f, 0.80f, 1.0f);

	TSharedRef<SWidget> Root =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(mRewardPanel, SBorder)
			.Padding(FMargin(36.0f, 28.0f))
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(PanelColor)
			[
				SNew(SBox)
				.WidthOverride(520.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 18.0f))
					[
						SAssignNew(mTitleText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 34))
						.ColorAndOpacity(TextColor)
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 10.0f))
					[
						SAssignNew(mGoldText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 25))
						.ColorAndOpacity(MutedTextColor)
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 24.0f))
					[
						SAssignNew(mExpText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 25))
						.ColorAndOpacity(MutedTextColor)
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SButton)
						.ContentPadding(FMargin(34.0f, 12.0f))
						.ButtonColorAndOpacity(ButtonColor)
						.OnClicked_UObject(this, &UCombatResultOverlayWidget::HandleConfirmClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Claim", "CLAIM"))
							.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 24))
							.ColorAndOpacity(TextColor)
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 84.0f))
		[
			SAssignNew(mContinuePanel, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Transparent)
			[
				SNew(SButton)
				.ContentPadding(FMargin(42.0f, 14.0f))
				.ButtonColorAndOpacity(ButtonColor)
				.OnClicked_UObject(this, &UCombatResultOverlayWidget::HandleConfirmClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Continue", "CONTINUE"))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 26))
					.ColorAndOpacity(TextColor)
					.Justification(ETextJustify::Center)
				]
			]
		];

	RefreshSlate();
	return Root;
}

FReply UCombatResultOverlayWidget::HandleConfirmClicked()
{
	FSimpleDelegate Callback = MoveTemp(mConfirmCallback);
	mConfirmCallback.Unbind();
	Callback.ExecuteIfBound();
	return FReply::Handled();
}

void UCombatResultOverlayWidget::RefreshSlate()
{
	if (mRewardPanel.IsValid())
	{
		mRewardPanel->SetVisibility(mMode == ECombatResultOverlayMode::VictoryReward ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (mContinuePanel.IsValid())
	{
		mContinuePanel->SetVisibility(mMode == ECombatResultOverlayMode::DefeatContinue ? EVisibility::Visible : EVisibility::Collapsed);
	}
	if (mTitleText.IsValid())
	{
		mTitleText->SetText(mReward.mTitle.IsEmpty() ? LOCTEXT("VictoryReward", "VICTORY REWARD") : mReward.mTitle);
	}
	if (mGoldText.IsValid())
	{
		mGoldText->SetText(FText::Format(
			LOCTEXT("GoldRewardFormat", "Gold +{0}  /  {1}"),
			FText::AsNumber(mReward.mGoldGained),
			FText::AsNumber(mReward.mGoldBalance)));
	}
	if (mExpText.IsValid())
	{
		mExpText->SetText(FText::Format(
			LOCTEXT("ExpRewardFormat", "EXP +{0}  /  Lv {1}"),
			FText::AsNumber(mReward.mExpGained),
			FText::AsNumber(mReward.mLevelAfter)));
	}
}

#undef LOCTEXT_NAMESPACE
