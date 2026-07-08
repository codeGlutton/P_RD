#include "UI/DicePanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

#define LOCTEXT_NAMESPACE "DicePanelWidget_Visuals"

namespace
{
	bool ShouldShowPip(TCHAR PipName, int32 ResultValue)
	{
		switch (ResultValue)
		{
		case 1:
			return PipName == TCHAR('C');
		case 2:
			return PipName == TCHAR('A') || PipName == TCHAR('E');
		case 3:
			return PipName == TCHAR('A') || PipName == TCHAR('C') || PipName == TCHAR('E');
		case 4:
			return PipName == TCHAR('A') || PipName == TCHAR('B') || PipName == TCHAR('D') || PipName == TCHAR('E');
		case 5:
			return PipName == TCHAR('A') || PipName == TCHAR('B') || PipName == TCHAR('C') || PipName == TCHAR('D') || PipName == TCHAR('E');
		default:
			return PipName == TCHAR('A') || PipName == TCHAR('B') || PipName == TCHAR('D') || PipName == TCHAR('E');
		}
	}
}

void UDicePanelWidget::RefreshDicePanelWidgets()
{
	EnsureDicePanelPreviewWidgets();

	if (UTextBlock* CarouselTitleText = FindDiceText(TEXT("CarouselTitleText")))
	{
		CarouselTitleText->SetText(FText::Format(
			LOCTEXT("DICE {0}", "DICE {0}"),
			FText::AsNumber(mDicePanelViews.Num())
		));
	}

	for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
	{
		const bool bShouldShowDice = mDicePanelViews.IsValidIndex(DiceIndex);
		if (UWidget* CarouselItem = FindDiceWidget(FString::Printf(TEXT("CarouselItem_%d"), DiceIndex)))
		{
			CarouselItem->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UWidget* DiceFaceSize = FindDiceWidget(FString::Printf(TEXT("DiceFaceSize_%d"), DiceIndex)))
		{
			DiceFaceSize->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (mDicePanelCardBorders.IsValidIndex(DiceIndex) && mDicePanelCardBorders[DiceIndex] != nullptr)
		{
			mDicePanelCardBorders[DiceIndex]->SetVisibility(bShouldShowDice ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (bShouldShowDice)
			{
				const FLinearColor RarityColor = RDUIDice::GetDiceRarityColor(mDicePanelViews[DiceIndex].mRarityType, RDUIDice::EDiceRarityColorTone::DicePanel);
				mDicePanelCardBorders[DiceIndex]->SetBrushColor(FLinearColor(RarityColor.R * 0.38f, RarityColor.G * 0.45f, RarityColor.B * 0.44f, 0.92f));
			}
		}
		if (mDicePanelImages.IsValidIndex(DiceIndex) && mDicePanelImages[DiceIndex] != nullptr)
		{
			mDicePanelImages[DiceIndex]->SetVisibility(bShouldShowDice ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	ApplyDicePanelLayout();
	RefreshDicePanelPreviewActors();
	RefreshDiceCarouselControlWidgets();
}

void UDicePanelWidget::ApplyDiceFaceVisuals(int32 DiceIndex, const FDiceViewData& DiceView)
{
	const FLinearColor RarityColor = RDUIDice::GetDiceRarityColor(DiceView.mRarityType, RDUIDice::EDiceRarityColorTone::DicePanel);
	const FLinearColor PendingColor(
		RarityColor.R * 0.58f,
		RarityColor.G * 0.58f,
		RarityColor.B * 0.58f,
		0.60f
	);
	const FLinearColor FaceColor = DiceView.mIsRolled ? RarityColor : PendingColor;

	if (UBorder* ItemFill = FindDiceBorder(FString::Printf(TEXT("CarouselItem_%d_Fill"), DiceIndex)))
	{
		ItemFill->SetBrushColor(FLinearColor(FaceColor.R * 0.40f, FaceColor.G * 0.48f, FaceColor.B * 0.46f, 0.78f));
	}
	if (UBorder* ItemLine = FindDiceBorder(FString::Printf(TEXT("CarouselItem_%d_Line"), DiceIndex)))
	{
		ItemLine->SetBrushColor(FaceColor);
	}
	if (UTextBlock* ItemAccent = FindDiceText(FString::Printf(TEXT("CarouselItem_%d_Accent"), DiceIndex)))
	{
		ItemAccent->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UBorder* DiceFaceFill = FindDiceBorder(FString::Printf(TEXT("DiceFaceFill_%d"), DiceIndex)))
	{
		DiceFaceFill->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UTextBlock* DiceLabel = FindDiceText(FString::Printf(TEXT("DiceLabel_%d"), DiceIndex)))
	{
		FSlateFontInfo LabelFont = DiceLabel->GetFont();
		LabelFont.Size = DiceView.mIsRolled ? 52 : 34;
		DiceLabel->SetFont(LabelFont);
		DiceLabel->SetJustification(ETextJustify::Center);
		DiceLabel->SetText(BuildDiceFaceText(DiceView));
		DiceLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.08f, 0.08f, 1.0f)));
		DiceLabel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mDicePanelImages.IsValidIndex(DiceIndex) && mDicePanelImages[DiceIndex] != nullptr)
	{
		mDicePanelImages[DiceIndex]->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	ApplyDicePipVisibility(DiceIndex, DiceView.mResultValue, false);
}

void UDicePanelWidget::ApplyDicePipVisibility(int32 DiceIndex, int32 ResultValue, bool IsRolled) const
{
	static constexpr TCHAR PipNames[] = { TCHAR('A'), TCHAR('B'), TCHAR('C'), TCHAR('D'), TCHAR('E') };
	for (TCHAR PipName : PipNames)
	{
		UBorder* DicePip = FindDiceBorder(FString::Printf(TEXT("DicePip%c_%d"), PipName, DiceIndex));
		if (DicePip == nullptr)
		{
			continue;
		}

		const bool bShowPip = IsRolled && ShouldShowPip(PipName, ResultValue);
		DicePip->SetVisibility(bShowPip ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		DicePip->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.07f, 1.0f));
	}

	UBorder* LeftMiddlePip = EnsureRuntimeDicePip(DiceIndex, TCHAR('F'), FVector2D(22.0f, 43.0f));
	UBorder* RightMiddlePip = EnsureRuntimeDicePip(DiceIndex, TCHAR('G'), FVector2D(64.0f, 43.0f));
	const bool bShowSixPip = IsRolled && ResultValue >= 6;
	if (LeftMiddlePip != nullptr)
	{
		LeftMiddlePip->SetVisibility(bShowSixPip ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		LeftMiddlePip->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.07f, 1.0f));
	}
	if (RightMiddlePip != nullptr)
	{
		RightMiddlePip->SetVisibility(bShowSixPip ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		RightMiddlePip->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.07f, 1.0f));
	}
}

UBorder* UDicePanelWidget::EnsureRuntimeDicePip(int32 DiceIndex, TCHAR PipName, const FVector2D& Position) const
{
	const FString PipWidgetName = FString::Printf(TEXT("DicePip%c_%d"), PipName, DiceIndex);
	if (UBorder* ExistingPip = FindDiceBorder(PipWidgetName))
	{
		return ExistingPip;
	}

	if (WidgetTree == nullptr)
	{
		return nullptr;
	}

	UCanvasPanel* DiceFace = Cast<UCanvasPanel>(FindDiceWidget(FString::Printf(TEXT("DiceFace_%d"), DiceIndex)));
	if (DiceFace == nullptr)
	{
		return nullptr;
	}

	UBorder* NewPip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(*PipWidgetName));
	if (NewPip == nullptr)
	{
		return nullptr;
	}

	DiceFace->AddChildToCanvas(NewPip);
	if (UCanvasPanelSlot* PipSlot = Cast<UCanvasPanelSlot>(NewPip->Slot))
	{
		PipSlot->SetAutoSize(false);
		PipSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		PipSlot->SetAlignment(FVector2D::ZeroVector);
		PipSlot->SetPosition(Position);
		PipSlot->SetSize(FVector2D(10.0f, 10.0f));
		PipSlot->SetZOrder(2);
	}

	return NewPip;
}

UWidget* UDicePanelWidget::FindDiceWidget(const FString& WidgetName) const
{
	return WidgetTree != nullptr ? WidgetTree->FindWidget(FName(*WidgetName)) : nullptr;
}

UBorder* UDicePanelWidget::FindDiceBorder(const FString& WidgetName) const
{
	return Cast<UBorder>(FindDiceWidget(WidgetName));
}

UTextBlock* UDicePanelWidget::FindDiceText(const FString& WidgetName) const
{
	return Cast<UTextBlock>(FindDiceWidget(WidgetName));
}

FText UDicePanelWidget::BuildDiceFaceText(const FDiceViewData& DiceView) const
{
	if (DiceView.mIsRolled)
	{
		return FText::AsNumber(DiceView.mResultValue);
	}

	return NSLOCTEXT("DicePanelWidget", "DiceNotRolledText", "...");
}

#undef LOCTEXT_NAMESPACE
