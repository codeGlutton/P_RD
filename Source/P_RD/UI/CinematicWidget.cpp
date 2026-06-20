// 이 파일: 시네마틱 위젯의 Slate UI 구성(영상 이미지 + 검은 배경 + "로딩중" 대기 레이어)과
//          수명 주기(재생 시작 PlayCinematic / 종료 FinishCinematic / 닫기 애니메이션 진입)를 담당.
#include "UI/CinematicWidget.h"

#include "MediaTexture.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	class SCinematicCoverImage : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCinematicCoverImage)
			: _Image(nullptr)
		{
		}

			SLATE_ARGUMENT(const FSlateBrush*, Image)
			SLATE_ATTRIBUTE(FVector2D, NativeSize)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Image = InArgs._Image;
			NativeSize = InArgs._NativeSize;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D::ZeroVector;
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			if (Image == nullptr || Image->DrawAs == ESlateBrushDrawType::NoDrawType)
			{
				return LayerId;
			}

			const FVector2D ScreenSize = AllottedGeometry.GetLocalSize();
			if (ScreenSize.X <= 0.0f || ScreenSize.Y <= 0.0f)
			{
				return LayerId;
			}

			FVector2D VideoSize = NativeSize.Get();
			if (VideoSize.X <= 0.0f || VideoSize.Y <= 0.0f)
			{
				VideoSize = Image->ImageSize;
			}
			if (VideoSize.X <= 0.0f || VideoSize.Y <= 0.0f)
			{
				VideoSize = FVector2D(1.0f, 1.0f);
			}

			const float CoverScale = FMath::Max(ScreenSize.X / VideoSize.X, ScreenSize.Y / VideoSize.Y);
			const FVector2D DrawSize = VideoSize * CoverScale;
			const FVector2D DrawOffset = (ScreenSize - DrawSize) * 0.5f;
			const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

			OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(DrawOffset)),
				Image,
				DrawEffects,
				InWidgetStyle.GetColorAndOpacityTint() * Image->GetTint(InWidgetStyle)
			);
			OutDrawElements.PopClip();

			return LayerId;
		}

	private:
		const FSlateBrush* Image = nullptr;
		TAttribute<FVector2D> NativeSize;
	};
}

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
	mCinematicVideoNativeSize = ReadCinematicVideoFileDimensions(ResolveCinematicVideoPath());
	mCinematicVideoBrush.ImageSize = mCinematicVideoNativeSize.X > 0.0f && mCinematicVideoNativeSize.Y > 0.0f
		? mCinematicVideoNativeSize
		: FVector2D(640.0f, 640.0f);
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
			// 화면 전체에 직접 그리되 원본 비율을 유지한 cover rect를 계산한다.
			// SImage는 할당 영역에 리소스를 그대로 늘려 그릴 수 있어 Android에서 정사각 영상이 4:3으로 찌그러졌다.
			SAssignNew(mCinematicVideoImage, SCinematicCoverImage)
			.Image(&mCinematicVideoBrush)
			.NativeSize(TAttribute<FVector2D>::CreateLambda([this]()
			{
				return mCinematicVideoNativeSize;
			}))
			.Clipping(EWidgetClipping::ClipToBounds)
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
