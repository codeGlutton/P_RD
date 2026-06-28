#include "UI/DicePanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/IndexedButtonWidget.h"
#include "UI/PanelNavigationStyle.h"
#include "UI/PanelRootWidgetUtils.h"

namespace
{
	using RDPanelWidgetUtils::FindOrCreateRootWidget;

	UTextBlock* FindOrCreateButtonText(UWidgetTree* WidgetTree, UButton* Button, FName Name, const FText& Text)
	{
		if (WidgetTree == nullptr || Button == nullptr)
		{
			return nullptr;
		}

		UTextBlock* ButtonText = Cast<UTextBlock>(WidgetTree->FindWidget(Name));
		if (ButtonText == nullptr)
		{
			ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		}
		if (ButtonText != nullptr)
		{
			ButtonText->SetJustification(ETextJustify::Center);
			ButtonText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetButtonTextColor()));
			ButtonText->SetText(Text);
			if (ButtonText->GetParent() == nullptr)
			{
				Button->AddChild(ButtonText);
			}
		}
		return ButtonText;
	}
}

void UDicePanelWidget::EnsureDiceCarouselControlWidgets()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	if (mRootCanvas == nullptr)
	{
		mRootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	if (mRootCanvas == nullptr)
	{
		return;
	}

	if (mDiceCarouselTitleText == nullptr)
	{
		mDiceCarouselTitleText = FindOrCreateRootWidget<UTextBlock>(WidgetTree, mRootCanvas, TEXT("DiceCarouselTitleText"));
		if (mDiceCarouselTitleText != nullptr)
		{
			mDiceCarouselTitleText->SetJustification(ETextJustify::Center);
			mDiceCarouselTitleText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
		}
	}

	if (mDiceCarouselFaceText == nullptr)
	{
		mDiceCarouselFaceText = FindOrCreateRootWidget<UTextBlock>(WidgetTree, mRootCanvas, TEXT("DiceCarouselFaceText"));
		if (mDiceCarouselFaceText != nullptr)
		{
			mDiceCarouselFaceText->SetJustification(ETextJustify::Center);
			mDiceCarouselFaceText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
		}
	}

	if (mDiceCarouselFaceButtons.Num() != DicePanelMaxFaceButtonCount)
	{
		mDiceCarouselFaceButtons.SetNum(DicePanelMaxFaceButtonCount);
	}
	if (mDiceCarouselFaceButtonTexts.Num() != DicePanelMaxFaceButtonCount)
	{
		mDiceCarouselFaceButtonTexts.SetNum(DicePanelMaxFaceButtonCount);
	}

	for (int32 FaceIndex = 0; FaceIndex < DicePanelMaxFaceButtonCount; ++FaceIndex)
	{
		if (mDiceCarouselFaceButtons[FaceIndex] != nullptr)
		{
			continue;
		}

		const int32 FaceValue = FaceIndex + 1;
		UIndexedButtonWidget* FaceButton = FindOrCreateRootWidget<UIndexedButtonWidget>(
			WidgetTree,
			mRootCanvas,
			FName(*FString::Printf(TEXT("DiceCarouselFaceButton_%d"), FaceValue))
		);
		if (FaceButton == nullptr)
		{
			continue;
		}

		UTextBlock* FaceButtonText = FindOrCreateButtonText(
			WidgetTree,
			FaceButton,
			FName(*FString::Printf(TEXT("DiceCarouselFaceButtonText_%d"), FaceValue)),
			FText::AsNumber(FaceValue)
		);
		if (FaceButtonText != nullptr)
		{
			FSlateFontInfo FaceButtonFont = FaceButtonText->GetFont();
			FaceButtonFont.Size = 28;
			FaceButtonText->SetFont(FaceButtonFont);
			mDiceCarouselFaceButtonTexts[FaceIndex] = FaceButtonText;
		}

		BindDiceCarouselFaceButton(FaceButton, FaceValue);
		FaceButton->SetButtonIndex(FaceValue);
		FaceButton->SetBackgroundColor(RDPanelNavigationStyle::GetOptionButtonColor());
		mDiceCarouselFaceButtons[FaceIndex] = FaceButton;
	}

	if (mDiceCarouselPreviousButton == nullptr)
	{
		mDiceCarouselPreviousButton = FindOrCreateRootWidget<UButton>(WidgetTree, mRootCanvas, TEXT("DiceCarouselPreviousButton"));
		if (mDiceCarouselPreviousButton != nullptr)
		{
			FindOrCreateButtonText(WidgetTree, mDiceCarouselPreviousButton, TEXT("DiceCarouselPreviousButtonText"), NSLOCTEXT("DicePanelWidget", "DiceCarouselPreviousText", "<"));
			mDiceCarouselPreviousButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mDiceCarouselPreviousButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselPreviousButtonClicked);
		}
	}

	if (mDiceCarouselNextButton == nullptr)
	{
		mDiceCarouselNextButton = FindOrCreateRootWidget<UButton>(WidgetTree, mRootCanvas, TEXT("DiceCarouselNextButton"));
		if (mDiceCarouselNextButton != nullptr)
		{
			FindOrCreateButtonText(WidgetTree, mDiceCarouselNextButton, TEXT("DiceCarouselNextButtonText"), NSLOCTEXT("DicePanelWidget", "DiceCarouselNextText", ">"));
			mDiceCarouselNextButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mDiceCarouselNextButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselNextButtonClicked);
		}
	}

	if (mDiceCarouselBackButton == nullptr)
	{
		mDiceCarouselBackButton = FindOrCreateRootWidget<UButton>(WidgetTree, mRootCanvas, TEXT("DiceCarouselBackButton"));
		if (mDiceCarouselBackButton != nullptr)
		{
			FindOrCreateButtonText(WidgetTree, mDiceCarouselBackButton, TEXT("DiceCarouselBackButtonText"), NSLOCTEXT("DicePanelWidget", "DiceCarouselBackText", "BACK"));
			mDiceCarouselBackButton->SetBackgroundColor(RDPanelNavigationStyle::GetBackButtonColor());
			mDiceCarouselBackButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselBackButtonClicked);
		}
	}

	ApplyDiceCarouselControlLayout();
}

void UDicePanelWidget::RefreshDiceCarouselControlWidgets()
{
	EnsureDiceCarouselControlWidgets();

	const bool bShowCarouselControls = mDicePanelCarouselActivated && mDicePanelViews.IsValidIndex(mSelectedDicePanelIndex);
	const ESlateVisibility ControlVisibility = bShowCarouselControls ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (mDiceCarouselTitleText != nullptr)
	{
		mDiceCarouselTitleText->SetVisibility(ControlVisibility);
	}
	if (mDiceCarouselFaceText != nullptr)
	{
		mDiceCarouselFaceText->SetVisibility(ControlVisibility);
	}
	const int32 FaceButtonCount = mDicePanelViews.IsValidIndex(mSelectedDicePanelIndex)
		? FMath::Clamp(mDicePanelViews[mSelectedDicePanelIndex].mFaceCount, 0, DicePanelMaxFaceButtonCount)
		: 0;
	for (int32 FaceIndex = 0; FaceIndex < mDiceCarouselFaceButtons.Num(); ++FaceIndex)
	{
		UIndexedButtonWidget* FaceButton = mDiceCarouselFaceButtons[FaceIndex];
		if (FaceButton != nullptr)
		{
			FaceButton->SetVisibility(bShowCarouselControls && FaceIndex < FaceButtonCount ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
	if (mDiceCarouselPreviousButton != nullptr)
	{
		mDiceCarouselPreviousButton->SetVisibility(bShowCarouselControls && mDicePanelViews.Num() > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (mDiceCarouselNextButton != nullptr)
	{
		mDiceCarouselNextButton->SetVisibility(bShowCarouselControls && mDicePanelViews.Num() > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (mDiceCarouselBackButton != nullptr)
	{
		mDiceCarouselBackButton->SetVisibility(ControlVisibility);
	}

	if (bShowCarouselControls == false)
	{
		return;
	}

	const FDiceViewData& DiceView = mDicePanelViews[mSelectedDicePanelIndex];
	if (mDiceCarouselTitleText != nullptr)
	{
		mDiceCarouselTitleText->SetText(FText::Format(
			NSLOCTEXT("DicePanelWidget", "DiceCarouselTitleFormat", "DICE {0} / {1}"),
			FText::AsNumber(mSelectedDicePanelIndex + 1),
			FText::AsNumber(mDicePanelViews.Num())
		));
	}
	if (mDiceCarouselFaceText != nullptr)
	{
		mDiceCarouselFaceText->SetText(FText::Format(
			NSLOCTEXT("DicePanelWidget", "DiceCarouselFaceFormat", "RESULT {0} / d{1}"),
			BuildDiceFaceText(DiceView),
			FText::AsNumber(DiceView.mFaceCount)
		));
	}

	for (int32 FaceIndex = 0; FaceIndex < FaceButtonCount; ++FaceIndex)
	{
		if (mDiceCarouselFaceButtonTexts.IsValidIndex(FaceIndex) && mDiceCarouselFaceButtonTexts[FaceIndex] != nullptr)
		{
			const int32 FaceDisplayValue = DiceView.mFaceValues.IsValidIndex(FaceIndex) ? DiceView.mFaceValues[FaceIndex] : FaceIndex + 1;
			mDiceCarouselFaceButtonTexts[FaceIndex]->SetText(FText::AsNumber(FaceDisplayValue));
		}
	}
	RefreshDiceCarouselFaceButtonStyles();
}

void UDicePanelWidget::RefreshDiceCarouselFaceButtonStyles()
{
	const int32 SelectedFaceValue = mDicePanelSelectedFaceValues.IsValidIndex(mSelectedDicePanelIndex)
		? mDicePanelSelectedFaceValues[mSelectedDicePanelIndex]
		: INDEX_NONE;
	const int32 FaceButtonCount = mDicePanelViews.IsValidIndex(mSelectedDicePanelIndex)
		? FMath::Clamp(mDicePanelViews[mSelectedDicePanelIndex].mFaceCount, 0, DicePanelMaxFaceButtonCount)
		: 0;

	for (int32 FaceIndex = 0; FaceIndex < mDiceCarouselFaceButtons.Num(); ++FaceIndex)
	{
		UIndexedButtonWidget* FaceButton = mDiceCarouselFaceButtons[FaceIndex];
		if (FaceButton == nullptr)
		{
			continue;
		}

		if (FaceIndex >= FaceButtonCount)
		{
			FaceButton->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const bool bIsSelectedFace = SelectedFaceValue == FaceIndex + 1;
		FaceButton->SetBackgroundColor(bIsSelectedFace
			? RDPanelNavigationStyle::GetSelectedButtonColor()
			: RDPanelNavigationStyle::GetOptionButtonColor());

		if (mDiceCarouselFaceButtonTexts.IsValidIndex(FaceIndex) && mDiceCarouselFaceButtonTexts[FaceIndex] != nullptr)
		{
			mDiceCarouselFaceButtonTexts[FaceIndex]->SetColorAndOpacity(FSlateColor(bIsSelectedFace
				? RDPanelNavigationStyle::GetSelectedButtonTextColor()
				: RDPanelNavigationStyle::GetButtonTextColor()));
		}
	}
}

