#include "UI/DicePanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "UI/IndexedButtonWidget.h"
#include "UI/PanelNavigationStyle.h"

namespace
{
	UTextBlock* BuildButtonText(UWidgetTree* WidgetTree, const TCHAR* Name, const FText& Text)
	{
		if (WidgetTree == nullptr)
		{
			return nullptr;
		}

		UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		if (ButtonText != nullptr)
		{
			ButtonText->SetJustification(ETextJustify::Center);
			ButtonText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetButtonTextColor()));
			ButtonText->SetText(Text);
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
		mDiceCarouselTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DiceCarouselTitleText"));
		if (mDiceCarouselTitleText != nullptr)
		{
			mDiceCarouselTitleText->SetJustification(ETextJustify::Center);
			mDiceCarouselTitleText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
			mRootCanvas->AddChildToCanvas(mDiceCarouselTitleText);
		}
	}

	if (mDiceCarouselFaceText == nullptr)
	{
		mDiceCarouselFaceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DiceCarouselFaceText"));
		if (mDiceCarouselFaceText != nullptr)
		{
			mDiceCarouselFaceText->SetJustification(ETextJustify::Center);
			mDiceCarouselFaceText->SetColorAndOpacity(FSlateColor(RDPanelNavigationStyle::GetPanelTextColor()));
			mRootCanvas->AddChildToCanvas(mDiceCarouselFaceText);
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
		UIndexedButtonWidget* FaceButton = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
			UIndexedButtonWidget::StaticClass(),
			FName(*FString::Printf(TEXT("DiceCarouselFaceButton_%d"), FaceValue))
		);
		if (FaceButton == nullptr)
		{
			continue;
		}

		UTextBlock* FaceButtonText = BuildButtonText(
			WidgetTree,
			*FString::Printf(TEXT("DiceCarouselFaceButtonText_%d"), FaceValue),
			FText::AsNumber(FaceValue)
		);
		if (FaceButtonText != nullptr)
		{
			FSlateFontInfo FaceButtonFont = FaceButtonText->GetFont();
			FaceButtonFont.Size = 28;
			FaceButtonText->SetFont(FaceButtonFont);
			FaceButton->AddChild(FaceButtonText);
			mDiceCarouselFaceButtonTexts[FaceIndex] = FaceButtonText;
		}

		BindDiceCarouselFaceButton(FaceButton, FaceValue);
		FaceButton->SetButtonIndex(FaceValue);
		FaceButton->SetBackgroundColor(RDPanelNavigationStyle::GetOptionButtonColor());
		mRootCanvas->AddChildToCanvas(FaceButton);
		mDiceCarouselFaceButtons[FaceIndex] = FaceButton;
	}

	if (mDiceCarouselPreviousButton == nullptr)
	{
		mDiceCarouselPreviousButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DiceCarouselPreviousButton"));
		if (mDiceCarouselPreviousButton != nullptr)
		{
			mDiceCarouselPreviousButton->AddChild(BuildButtonText(WidgetTree, TEXT("DiceCarouselPreviousButtonText"), NSLOCTEXT("DicePanelWidget", "DiceCarouselPreviousText", "<")));
			mDiceCarouselPreviousButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mDiceCarouselPreviousButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselPreviousButtonClicked);
			mRootCanvas->AddChildToCanvas(mDiceCarouselPreviousButton);
		}
	}

	if (mDiceCarouselNextButton == nullptr)
	{
		mDiceCarouselNextButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DiceCarouselNextButton"));
		if (mDiceCarouselNextButton != nullptr)
		{
			mDiceCarouselNextButton->AddChild(BuildButtonText(WidgetTree, TEXT("DiceCarouselNextButtonText"), NSLOCTEXT("DicePanelWidget", "DiceCarouselNextText", ">")));
			mDiceCarouselNextButton->SetBackgroundColor(RDPanelNavigationStyle::GetNavigationButtonColor());
			mDiceCarouselNextButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselNextButtonClicked);
			mRootCanvas->AddChildToCanvas(mDiceCarouselNextButton);
		}
	}

	if (mDiceCarouselBackButton == nullptr)
	{
		mDiceCarouselBackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DiceCarouselBackButton"));
		if (mDiceCarouselBackButton != nullptr)
		{
			mDiceCarouselBackButton->AddChild(BuildButtonText(WidgetTree, TEXT("DiceCarouselBackButtonText"), NSLOCTEXT("DicePanelWidget", "DiceCarouselBackText", "BACK")));
			mDiceCarouselBackButton->SetBackgroundColor(RDPanelNavigationStyle::GetBackButtonColor());
			mDiceCarouselBackButton->OnClicked.AddUniqueDynamic(this, &UDicePanelWidget::HandleDiceCarouselBackButtonClicked);
			mRootCanvas->AddChildToCanvas(mDiceCarouselBackButton);
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

