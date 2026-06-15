#include "UI/CinematicWidget.h"

#include "MediaTexture.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

UCinematicWidget::UCinematicWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mRemoveFromParentOnClose = true;
}

TSharedRef<SWidget> UCinematicWidget::RebuildWidget()
{
	EnsureCinematicMediaObjects();

	mCinematicVideoBrush = FSlateBrush();
	mCinematicVideoBrush.DrawAs = ESlateBrushDrawType::Image;
	mCinematicVideoBrush.ImageSize = FVector2D(1280.0f, 720.0f);
	mCinematicVideoBrush.SetResourceObject(mCinematicMediaTexture);

	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::Both)
			[
				SAssignNew(mCinematicVideoImage, SImage)
				.Image(&mCinematicVideoBrush)
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(mLoadingWaitLayer, SBorder)
			.Visibility(EVisibility::Collapsed)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNullWidget::NullWidget
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 96.0f))
				[
					SAssignNew(mLoadingWaitText, STextBlock)
					.Text(NSLOCTEXT("CinematicWidget", "LoadingWaitText", "로딩중"))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 32))
					.ColorAndOpacity(FLinearColor::Transparent)
					.Justification(ETextJustify::Center)
				]
			]
		];
}

void UCinematicWidget::NativeDestruct()
{
	ClearDefaultCinematicTimer();
	ClearCinematicFadeTimer();

	StopCinematicMedia();

	Super::NativeDestruct();
}

void UCinematicWidget::PlayCinematic(FOnEndCinematicAnimation Callback)
{
	if (IsOpened() == false)
	{
		return;
	}

	OnEndCinematicAnimation = MoveTemp(Callback);
	mCinematicFinished = false;
	ClearDefaultCinematicTimer();
	ClearCinematicFadeTimer();
	HideLoadingWaitScreen();
	PlayCinematicAnimation();
}

void UCinematicWidget::FinishCinematic()
{
	if (mCinematicFinished)
	{
		return;
	}

	mCinematicFinished = true;
	ClearDefaultCinematicTimer();

	if (OnEndCinematicAnimation.IsBound())
	{
		OnEndCinematicAnimation.Execute(this);
		OnEndCinematicAnimation.Unbind();
	}
}

void UCinematicWidget::FadeToLoadingWaitScreen()
{
	if (IsOpened() == false)
	{
		return;
	}

	if (mLoadingWaitLayerOpacity >= 1.0f)
	{
		ShowLoadingWaitScreen();
		return;
	}

	StartFadeToBlack(ECinematicFadePurpose::LoadingWait);
}

void UCinematicWidget::PlayCinematicAnimation_Implementation()
{
	if (PlayCinematicVideo() == false)
	{
		StartDefaultCinematicTimer(mDefaultCinematicDuration);
	}
}

void UCinematicWidget::PlayCloseUIAnimation_Implementation()
{
	if (mLoadingWaitLayerOpacity >= 1.0f)
	{
		StopCinematicMedia();
		FinishCloseUI();
		return;
	}

	StartFadeToBlack(ECinematicFadePurpose::Close);
}
