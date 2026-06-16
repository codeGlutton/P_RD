// 이 파일: "로딩중" 대기 화면(검은 레이어 + 텍스트)의 표시/숨김과 레이어 투명도 제어.
//          페이드가 끝나 검은 화면이 되면 여기서 텍스트를 띄우고, 인트로로 돌아가면 다시 숨긴다.
#include "UI/CinematicWidget.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

void UCinematicWidget::ShowLoadingWaitScreen()
{
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
