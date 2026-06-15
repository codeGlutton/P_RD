#include "UI/TitleMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/TitleMenuButtonPolicy.h"
#include "UI/UIRuntimeLayout.h"
#include "UI/UITextureLoader.h"

void UTitleMenuWidget::ApplyTitleButtonStyles()
{
	if (mButtonRuntime.mMainTexture == nullptr)
	{
		mButtonRuntime.mMainTexture = RDUITexture::LoadTextureFromContentPng(mMainButtonImagePath, TEXT("TitleMenuWidget"));
	}

	if (mButtonRuntime.mSettingsTexture == nullptr)
	{
		mButtonRuntime.mSettingsTexture = RDUITexture::LoadTextureFromContentPng(mSettingsButtonImagePath, TEXT("TitleMenuWidget"));
	}

	EnsureTitleButtonBackgroundImages();
	ApplyTitleButtonImageBrush(mButtonRuntime.mContinueBackgroundImage, mButtonRuntime.mMainTexture);
	ApplyTitleButtonImageBrush(mButtonRuntime.mStartBackgroundImage, mButtonRuntime.mMainTexture);
	ApplyTitleButtonImageBrush(mButtonRuntime.mSettingsBackgroundImage, mButtonRuntime.mSettingsTexture);

	ApplyTextureButtonStyle(StartButton, mButtonRuntime.mMainTexture, mButtonRuntime.mMainBrush);
	ApplyTextureButtonStyle(ContinueButton, mButtonRuntime.mMainTexture, mButtonRuntime.mMainBrush);
	ApplyTextureButtonStyle(SettingsButton, mButtonRuntime.mSettingsTexture, mButtonRuntime.mSettingsBrush);
	ApplyTransparentButtonStyle(StartButton);
	ApplyTransparentButtonStyle(ContinueButton);
	ApplyTransparentButtonStyle(SettingsButton);
	ApplyTitleButtonLayouts();
}

void UTitleMenuWidget::RefreshTitleButtonBackgroundVisibility(bool bCanContinueRun) const
{
	if (mButtonRuntime.mStartBackgroundImage != nullptr)
	{
		mButtonRuntime.mStartBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (mButtonRuntime.mContinueBackgroundImage != nullptr)
	{
		mButtonRuntime.mContinueBackgroundImage->SetVisibility(bCanContinueRun ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (mButtonRuntime.mSettingsBackgroundImage != nullptr)
	{
		mButtonRuntime.mSettingsBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UTitleMenuWidget::EnsureTitleButtonBackgroundImages()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	if (mButtonRuntime.mContinueBackgroundImage == nullptr)
	{
		mButtonRuntime.mContinueBackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RuntimeContinueButtonBackgroundImage"));
		mButtonRuntime.mContinueBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (mButtonRuntime.mStartBackgroundImage == nullptr)
	{
		mButtonRuntime.mStartBackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RuntimeStartButtonBackgroundImage"));
		mButtonRuntime.mStartBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (mButtonRuntime.mSettingsBackgroundImage == nullptr)
	{
		mButtonRuntime.mSettingsBackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RuntimeSettingsButtonBackgroundImage"));
		mButtonRuntime.mSettingsBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UTitleMenuWidget::ApplyTitleButtonLayouts()
{
	TArray<RDTitleMenuButton::FButtonLayout> ButtonLayouts;
	RDTitleMenuButton::BuildButtonLayouts(OUT ButtonLayouts);

	for (const RDTitleMenuButton::FButtonLayout& ButtonLayout : ButtonLayouts)
	{
		UButton* TargetButton = nullptr;
		UImage* TargetImage = nullptr;
		switch (ButtonLayout.mSlot)
		{
		case RDTitleMenuButton::EButtonSlot::Continue:
			TargetButton = ContinueButton;
			TargetImage = mButtonRuntime.mContinueBackgroundImage;
			break;
		case RDTitleMenuButton::EButtonSlot::Start:
			TargetButton = StartButton;
			TargetImage = mButtonRuntime.mStartBackgroundImage;
			break;
		case RDTitleMenuButton::EButtonSlot::Settings:
			TargetButton = SettingsButton;
			TargetImage = mButtonRuntime.mSettingsBackgroundImage;
			break;
		}

		AttachTitleButtonBackgroundImage(
			TargetImage,
			ButtonLayout.mPosition,
			ButtonLayout.mSize,
			RDTitleMenuButton::GetBackgroundZOrder()
		);

		ApplyTitleButtonLayout(TargetButton, ButtonLayout.mSize);
		RDUILayout::ApplyFixedSlot(
			TargetButton,
			RDTitleMenuButton::GetCanvasAnchors(),
			RDTitleMenuButton::GetCanvasAlignment(),
			ButtonLayout.mPosition,
			ButtonLayout.mSize,
			RDTitleMenuButton::GetInputZOrder()
		);
	}
}

void UTitleMenuWidget::ApplyTextureButtonStyle(UButton* TargetButton, UTexture2D* Texture, FSlateBrush& InOutBrush) const
{
	if (TargetButton == nullptr || Texture == nullptr)
	{
		return;
	}

	InOutBrush = FSlateBrush();
	InOutBrush.DrawAs = ESlateBrushDrawType::Image;
	InOutBrush.ImageSize = FVector2D(
		static_cast<float>(Texture->GetSizeX()),
		static_cast<float>(Texture->GetSizeY()));
	InOutBrush.SetResourceObject(Texture);

	FSlateBrush HoveredBrush = InOutBrush;
	HoveredBrush.TintColor = FSlateColor(RDTitleMenuButton::GetHoveredTintColor());

	FSlateBrush PressedBrush = InOutBrush;
	PressedBrush.TintColor = FSlateColor(RDTitleMenuButton::GetPressedTintColor());

	FSlateBrush DisabledBrush = InOutBrush;
	DisabledBrush.TintColor = FSlateColor(RDTitleMenuButton::GetDisabledTintColor());

	FButtonStyle ButtonStyle = TargetButton->GetStyle();
	ButtonStyle.SetNormal(InOutBrush);
	ButtonStyle.SetHovered(HoveredBrush);
	ButtonStyle.SetPressed(PressedBrush);
	ButtonStyle.SetDisabled(DisabledBrush);
	ButtonStyle.SetNormalPadding(RDTitleMenuButton::GetNormalPadding());
	ButtonStyle.SetPressedPadding(RDTitleMenuButton::GetPressedPadding());
	TargetButton->SetStyle(ButtonStyle);
}

void UTitleMenuWidget::ApplyTitleButtonImageBrush(UImage* TargetImage, UTexture2D* Texture) const
{
	if (TargetImage == nullptr || Texture == nullptr)
	{
		return;
	}

	FSlateBrush ButtonImageBrush;
	ButtonImageBrush.DrawAs = ESlateBrushDrawType::Image;
	ButtonImageBrush.ImageSize = FVector2D(
		static_cast<float>(Texture->GetSizeX()),
		static_cast<float>(Texture->GetSizeY()));
	ButtonImageBrush.SetResourceObject(Texture);
	TargetImage->SetBrush(ButtonImageBrush);
}

bool UTitleMenuWidget::AttachTitleButtonBackgroundImage(UImage* TargetImage, const FVector2D& Position, const FVector2D& DesiredSize, int32 ZOrder)
{
	if (TargetImage == nullptr)
	{
		return false;
	}

	if (TargetImage->Slot == nullptr)
	{
		if (UCanvasPanel* StartCanvas = Cast<UCanvasPanel>(StartScreen))
		{
			StartCanvas->AddChildToCanvas(TargetImage);
		}
		else if (WidgetTree != nullptr)
		{
			UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
			if (RootCanvas != nullptr)
			{
				RootCanvas->AddChildToCanvas(TargetImage);
			}
		}
	}

	UCanvasPanelSlot* ImageSlot = Cast<UCanvasPanelSlot>(TargetImage->Slot);
	if (ImageSlot == nullptr)
	{
		return false;
	}

	RDUILayout::ApplyFixedSlot(TargetImage, RDTitleMenuButton::GetCanvasAnchors(), RDTitleMenuButton::GetCanvasAlignment(), Position, DesiredSize, ZOrder);
	return true;
}

void UTitleMenuWidget::ApplyTitleButtonLayout(UButton* TargetButton, const FVector2D& DesiredSize) const
{
	if (TargetButton == nullptr)
	{
		return;
	}

	if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(TargetButton->Slot))
	{
		ButtonSlot->SetSize(DesiredSize);
		TargetButton->SetRenderScale(FVector2D::UnitVector);
		return;
	}

	// CanvasSlot이 아닌 WBP 구조에서도 원본 버튼 이미지가 납작해지지 않도록 최소한의 보정만 둔다.
	TargetButton->SetRenderTransformPivot(RDTitleMenuButton::GetFallbackButtonPivot());
	TargetButton->SetRenderScale(RDTitleMenuButton::GetFallbackButtonScale());
}

void UTitleMenuWidget::ApplyTransparentButtonStyle(UButton* TargetButton) const
{
	if (TargetButton == nullptr)
	{
		return;
	}

	FSlateBrush TransparentBrush;
	TransparentBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	TransparentBrush.TintColor = FSlateColor(FLinearColor::Transparent);

	FButtonStyle TransparentStyle = TargetButton->GetStyle();
	TransparentStyle.SetNormal(TransparentBrush);
	TransparentStyle.SetHovered(TransparentBrush);
	TransparentStyle.SetPressed(TransparentBrush);
	TransparentStyle.SetDisabled(TransparentBrush);
	TransparentStyle.SetNormalPadding(RDTitleMenuButton::GetNormalPadding());
	TransparentStyle.SetPressedPadding(RDTitleMenuButton::GetPressedPadding());
	TargetButton->SetStyle(TransparentStyle);
}

void UTitleMenuWidget::ApplyTitleButtonTextStyle() const
{
	UTextBlock* ButtonTexts[] = {
		StartButtonText,
		ContinueButtonText,
		SettingsButtonText,
	};

	for (UTextBlock* ButtonText : ButtonTexts)
	{
		if (ButtonText == nullptr)
		{
			continue;
		}

		ButtonText->SetColorAndOpacity(RDTitleMenuButton::GetButtonTextColor());
		ButtonText->SetShadowColorAndOpacity(RDTitleMenuButton::GetButtonShadowColor());
		ButtonText->SetShadowOffset(RDTitleMenuButton::GetButtonShadowOffset());
	}
}
