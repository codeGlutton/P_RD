#include "UI/CinematicWidget.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

void UCinematicWidget::StartFadeToBlack(ECinematicFadePurpose FadePurpose)
{
	/*
	 * 같은 검은 페이드 레이어를 두 목적에 재사용한다.
	 * LoadingWait은 영상 종료 후 저장 로드를 기다리는 화면이고, Close는 인트로 위젯을 닫기 전 정리 페이드다.
	 */
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

	// 페이드가 끝난 뒤의 행동은 시작 목적에 따라 갈라진다. 타이머 틱에서는 alpha만 책임진다.
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
