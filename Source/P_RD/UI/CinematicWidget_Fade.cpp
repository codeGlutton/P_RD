// 이 파일: 영상이 끝난 뒤 화면을 검게 페이드하는 애니메이션(60fps 타이머로 투명도 0→1).
//          용도 두 가지 — ① 프론트엔드 로드가 늦으면 "로딩중" 대기 화면으로(LoadingWait)
//                          ② 이미 로드됐으면 인트로를 닫고 타이틀로(Close). FadePurpose로 구분.
#include "UI/CinematicWidget.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

void UCinematicWidget::StartFadeToBlack(ECinematicFadePurpose FadePurpose)
{
	if (mFadeOutDuration <= 0.0f)
	{
		mFadePurpose = FadePurpose;
		SetLoadingWaitLayerOpacity(1.0f);
		FinishFadeToBlack();
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		mFadePurpose = FadePurpose;
		SetLoadingWaitLayerOpacity(1.0f);
		FinishFadeToBlack();
		return;
	}

	ClearCinematicFadeTimer();
	mFadePurpose = FadePurpose;
	mFadeElapsedTime = mLoadingWaitLayerOpacity * mFadeOutDuration;

	if (mLoadingWaitLayer.IsValid())
	{
		mLoadingWaitLayer->SetVisibility(EVisibility::Visible);
	}
	if (mLoadingWaitText.IsValid())
	{
		mLoadingWaitText->SetVisibility(EVisibility::Collapsed);
	}

	World->GetTimerManager().SetTimer(
		mCinematicFadeTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			TickFadeToBlack();
		}),
		1.0f / 60.0f,
		true
	);
}

void UCinematicWidget::TickFadeToBlack()
{
	UWorld* World = GetWorld();
	const float DeltaSeconds = World != nullptr ? World->GetDeltaSeconds() : 1.0f / 60.0f;
	mFadeElapsedTime += DeltaSeconds;

	const float FadeAlpha = FMath::Clamp(mFadeElapsedTime / mFadeOutDuration, 0.0f, 1.0f);
	SetLoadingWaitLayerOpacity(FadeAlpha);

	if (FadeAlpha >= 1.0f)
	{
		FinishFadeToBlack();
	}
}

void UCinematicWidget::FinishFadeToBlack()
{
	ClearCinematicFadeTimer();
	SetLoadingWaitLayerOpacity(1.0f);

	if (mCinematicVideoImage.IsValid())
	{
		mCinematicVideoImage->SetVisibility(EVisibility::Collapsed);
	}

	const ECinematicFadePurpose FinishedFadePurpose = mFadePurpose;
	mFadePurpose = ECinematicFadePurpose::None;

	if (FinishedFadePurpose == ECinematicFadePurpose::LoadingWait)
	{
		ShowLoadingWaitScreen();
		return;
	}

	if (FinishedFadePurpose == ECinematicFadePurpose::Close)
	{
		StopCinematicMedia();
		FinishCloseUI();
	}
}

void UCinematicWidget::ClearCinematicFadeTimer()
{
	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(mCinematicFadeTimerHandle);
	}
	mCinematicFadeTimerHandle.Invalidate();
}
