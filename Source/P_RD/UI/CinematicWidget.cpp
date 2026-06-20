// 이 파일: 시네마틱 위젯의 Slate UI 구성(영상 이미지 + 검은 배경 + "로딩중" 대기 레이어)과
//          수명 주기(재생 시작 PlayCinematic / 종료 FinishCinematic / 닫기 애니메이션 진입)를 담당.
#include "UI/CinematicWidget.h"

#include "MediaTexture.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
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
			// 다양한 해상도에서 비율을 유지한 채 화면을 꽉 채우고(cover) 넘치는 부분만 잘라낸다.
			// 클립 박스가 화면을 채우며 자식을 중앙 정렬·클리핑하고, cover 박스는 영상 실제 비율로
			// 화면보다 크게 잡힌다(넘친 위/아래 또는 좌/우가 잘림). 크기는 NativeTick에서 갱신.
			SAssignNew(mCinematicVideoClipBox, SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Clipping(EWidgetClipping::ClipToBounds)
			[
				SAssignNew(mCinematicVideoCoverBox, SBox)
				[
					SAssignNew(mCinematicVideoImage, SImage)
					.Image(&mCinematicVideoBrush)
				]
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

/** @details 영상 표시 영역은 매 프레임 화면 크기/영상 해상도에 맞춰 cover 크기를 다시 계산해야 하므로 틱에서 갱신한다. */
void UCinematicWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateCinematicVideoCoverSize();
}

/** @details cover = 화면을 완전히 덮는 최소 배율(가로/세로 배율 중 큰 값)로 영상 박스를 키운다.
 *  같은 배율을 양 축에 적용하므로 비율이 보존되고, 넘친 부분은 클립 박스가 잘라낸다.
 *  영상 실제 해상도(미디어 텍스처)를 우선 쓰고, 아직 없으면 브러시 기본 크기를 폴백으로 쓴다. */
void UCinematicWidget::UpdateCinematicVideoCoverSize()
{
	if (mCinematicVideoClipBox.IsValid() == false || mCinematicVideoCoverBox.IsValid() == false)
	{
		return;
	}

	const FVector2D ScreenSize = mCinematicVideoClipBox->GetCachedGeometry().GetLocalSize();
	if (ScreenSize.X <= 0.0f || ScreenSize.Y <= 0.0f)
	{
		return;
	}

	FVector2D VideoSize = mCinematicVideoBrush.ImageSize;
	if (mCinematicMediaTexture != nullptr)
	{
		const float TextureWidth = static_cast<float>(mCinematicMediaTexture->GetWidth());
		const float TextureHeight = static_cast<float>(mCinematicMediaTexture->GetHeight());
		if (TextureWidth > 0.0f && TextureHeight > 0.0f)
		{
			VideoSize = FVector2D(TextureWidth, TextureHeight);
		}
	}
	if (VideoSize.X <= 0.0f || VideoSize.Y <= 0.0f)
	{
		return;
	}

	const float CoverScale = FMath::Max(ScreenSize.X / VideoSize.X, ScreenSize.Y / VideoSize.Y);
	mCinematicVideoCoverBox->SetWidthOverride(VideoSize.X * CoverScale);
	mCinematicVideoCoverBox->SetHeightOverride(VideoSize.Y * CoverScale);
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
