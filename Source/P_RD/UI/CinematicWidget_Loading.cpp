#include "UI/CinematicWidget.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

void UCinematicWidget::ShowLoadingWaitScreen()
{
	/*
	 * 인트로 영상 자체가 로딩 화면 역할을 하므로 별도 LoadingNotify 위젯을 띄우지 않는다.
	 * 영상이 먼저 끝났을 때만 같은 위젯 안의 검은 대기 레이어를 보여준다.
	 */
	ClearCinematicFadeTimer();
	SetLoadingWaitLayerOpacity(1.0f);

	if (mCinematicVideoImage.IsValid())
	{
		mCinematicVideoImage->SetVisibility(EVisibility::Collapsed);
	}

	if (mLoadingWaitLayer.IsValid())
	{
		mLoadingWaitLayer->SetVisibility(EVisibility::Visible);
	}
	if (mLoadingWaitText.IsValid())
	{
		mLoadingWaitText->SetVisibility(EVisibility::Visible);
		mLoadingWaitText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}

void UCinematicWidget::HideLoadingWaitScreen()
{
	SetLoadingWaitLayerOpacity(0.0f);
	mFadePurpose = ECinematicFadePurpose::None;

	if (mCinematicVideoImage.IsValid())
	{
		mCinematicVideoImage->SetVisibility(EVisibility::Visible);
	}

	if (mLoadingWaitLayer.IsValid())
	{
		mLoadingWaitLayer->SetVisibility(EVisibility::Collapsed);
	}
	if (mLoadingWaitText.IsValid())
	{
		mLoadingWaitText->SetVisibility(EVisibility::Collapsed);
		mLoadingWaitText->SetColorAndOpacity(FSlateColor(FLinearColor::Transparent));
	}
}

void UCinematicWidget::SetLoadingWaitLayerOpacity(float Opacity)
{
	mLoadingWaitLayerOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);

	if (mLoadingWaitLayer.IsValid())
	{
		mLoadingWaitLayer->SetVisibility(mLoadingWaitLayerOpacity > 0.0f ? EVisibility::Visible : EVisibility::Collapsed);
		mLoadingWaitLayer->SetBorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, mLoadingWaitLayerOpacity));
	}
}
