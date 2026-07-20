#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Kismet/GameplayStatics.h"   // 승리/패배 결과 징글 재생
#include "TimerManager.h"             // 결과 영상 시작 텀 타이머
#include "Setting/GamePlaySettings.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/CinematicWidget.h"
#include "UI/CombatResultOverlayWidget.h"
#include "UI/FrontendMapWidget.h"
#include "UI/Reward/RewardUIModel.h"
#include "UI/Reward/RewardUIWidgetBase.h"

namespace
{
	const TCHAR* const FallbackVictoryVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Victory_01.mp4");
	const TCHAR* const FallbackDefeatVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Defeat_01.mp4");
	constexpr int32 CombatResultVideoZOrder = -20;
	/** @brief 승/패 판정 후 결과 영상 시작까지의 텀(전장을 잠깐 보여주는 시간). */
	constexpr float CombatResultStartDelaySeconds = 1.2f;
}

void UCombatTileMapHUDWidget::BeginCombatResultPresentation(TSharedPtr<FPresentationBarrier> Barrier, bool IsPlayerWin)
{
	mIsPlayerWin = IsPlayerWin;
	mCombatResultBarrier = MoveTemp(Barrier);
	mVictoryWorldMapLocked = false;

	// 판정 직후 바로 영상이 틀어지면 급작스러워서, 전장을 잠깐 보여주는 텀을 두고 연출을 시작한다.
	// 배리어는 이미 붙잡고 있으므로 딜레이 동안 프레임워크 진행은 멈춰 있다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			mCombatResultStartDelayTimerHandle, this, &UCombatTileMapHUDWidget::StartCombatResultCinematic,
			CombatResultStartDelaySeconds, false);
		return;
	}

	StartCombatResultCinematic();
}

/** @brief 텀(딜레이) 후 실제 결과 연출 시작 — 징글 재생 + 결과 영상 오픈. */
void UCombatTileMapHUDWidget::StartCombatResultCinematic()
{
	// 결과 징글: 승리/패배에 맞는 짧은 음악을 결과 연출 시작과 동시에 1회 재생한다.
	USoundBase* ResultJingle = mIsPlayerWin == true ? mVictoryJingleSound.Get() : mDefeatJingleSound.Get();
	if (ResultJingle != nullptr)
	{
		UGameplayStatics::PlaySound2D(this, ResultJingle);
	}

	EnsureCombatResultWidgets();
	SetCombatResultViewActive(true);

	if (mCombatResultCinematicWidget == nullptr)
	{
		HandleCombatResultVideoFinished(nullptr);
		return;
	}

	mCombatResultCinematicWidget->SetCinematicViewportZOrder(CombatResultVideoZOrder);
	mCombatResultCinematicWidget->SetHoldLastFrameOnFinish(true);
	mCombatResultCinematicWidget->SetCinematicVideoPath(GetCombatResultVideoPath(mIsPlayerWin));
	mCombatResultCinematicWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(this, [this](UUserWidget* OpenedWidget)
	{
		if (UCinematicWidget* OpenedCinematicWidget = Cast<UCinematicWidget>(OpenedWidget))
		{
			OpenedCinematicWidget->PlayCinematic(FOnEndCinematicAnimation::CreateUObject(this, &UCombatTileMapHUDWidget::HandleCombatResultVideoFinished));
		}
	}));
}

void UCombatTileMapHUDWidget::EnsureCombatResultWidgets()
{
	if (mCombatResultCinematicWidget == nullptr)
	{
		mCombatResultCinematicWidget = CreateWidget<UCinematicWidget>(GetOwningPlayer(), UCinematicWidget::StaticClass());
	}

	if (mCombatResultOverlayWidget == nullptr)
	{
		mCombatResultOverlayWidget = CreateWidget<UCombatResultOverlayWidget>(GetOwningPlayer(), UCombatResultOverlayWidget::StaticClass());
	}

	if (mCombatRewardWidget == nullptr && mRewardWidgetClass != nullptr)
	{
		mCombatRewardWidget = CreateWidget<URewardUIWidgetBase>(GetOwningPlayer(), mRewardWidgetClass);
	}

	if (mCombatRewardWidget != nullptr)
	{
		mCombatRewardWidget->OnClosed.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatResultRewardConfirmed);
		mCombatRewardWidget->OnClosed.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatResultRewardConfirmed);
		mCombatRewardWidget->BindUIModel(mCombatRewardUIModel);
	}
}

void UCombatTileMapHUDWidget::HandleCombatResultVideoFinished(UCinematicWidget* CinematicWidget)
{
	if (mCombatResultFlowActive == false)
	{
		return;
	}

	EnsureCombatResultWidgets();
	if (mCombatResultOverlayWidget == nullptr)
	{
		SetCombatResultViewActive(false, true);
	}
	mCombatResultBarrier.Reset();
}

void UCombatTileMapHUDWidget::HandleCombatResultOpenRequested()
{
	if (mIsPlayerWin == true)
	{
		if (mCombatRewardWidget != nullptr && mCombatRewardUIModel != nullptr)
		{
			mCombatRewardWidget->OpenUI();
		}
		return;
	}

	mCombatResultOverlayWidget->ShowDefeatContinue(
		FSimpleDelegate::CreateUObject(this, &UCombatTileMapHUDWidget::HandleCombatResultContinueConfirmed));
	mCombatResultOverlayWidget->OpenUI();
}

void UCombatTileMapHUDWidget::HandleCombatResultRewardConfirmed()
{
	if (mCombatResultOverlayWidget != nullptr)
	{
		mCombatResultOverlayWidget->CloseUI();
	}

	mVictoryWorldMapLocked = true;

	CloseCombatResultCinematic(FSimpleDelegate::CreateWeakLambda(this, [this]() mutable
	{
		SetCombatResultViewActive(false, false);
		OpenWorldMapAfterPlayerWin();
	}));
}

void UCombatTileMapHUDWidget::HandleCombatRewardClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	if (mCombatRewardWidget != nullptr)
	{
		// 경험치 수령은 전용 상승음(골드는 카운트업 코인 사운드가 담당).
		if (ClaimKind == ERewardClaimKind::Exp && mExpGainSound != nullptr)
		{
			UGameplayStatics::PlaySound2D(this, mExpGainSound);
		}
		mCombatRewardWidget->NotifyRewardClaimed(ClaimKind, ChoiceIndex);
	}
}

void UCombatTileMapHUDWidget::HandleCombatResultContinueConfirmed()
{
	if (mCombatResultOverlayWidget != nullptr)
	{
		mCombatResultOverlayWidget->CloseUI();
	}

	CloseCombatResultCinematic(FSimpleDelegate::CreateWeakLambda(this, [this]() mutable
	{
		SetCombatResultViewActive(false, true);
		
		if (mCombatUIModel != nullptr)
		{
			mCombatUIModel->RequestAbandonRun();
		}
	}));
}

void UCombatTileMapHUDWidget::CloseCombatResultCinematic(FSimpleDelegate Callback)
{
	if (mCombatResultCinematicWidget != nullptr && mCombatResultCinematicWidget->IsOpened())
	{
		mCombatResultCinematicWidget->CloseUI(FOnEndUICloseAnimation::CreateWeakLambda(this, [Callback](UUserWidget*) mutable
		{
			Callback.ExecuteIfBound();
		}));
		return;
	}

	Callback.ExecuteIfBound();
}

void UCombatTileMapHUDWidget::SetCombatResultViewActive(bool bActive, bool bRestoreCombatControls)
{
	if (bActive)
	{
		mCombatResultFlowActive = true;
		CloseFloatingPanels(EWorldWidgetType::Count);
		SetCombatPlayControlsVisible(false);
		return;
	}

	if (bRestoreCombatControls)
	{
		SetCombatPlayControlsVisible(true);
	}
	mCombatResultFlowActive = false;
}

FString UCombatTileMapHUDWidget::GetCombatResultVideoPath(bool IsPlayerWin) const
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	if (IsPlayerWin == true)
	{
		return GamePlaySettings != nullptr && GamePlaySettings->mCombatVictoryVideoPath.IsEmpty() == false
			? GamePlaySettings->mCombatVictoryVideoPath
			: FString(FallbackVictoryVideoPath);
	}

	return GamePlaySettings != nullptr && GamePlaySettings->mCombatDefeatVideoPath.IsEmpty() == false
		? GamePlaySettings->mCombatDefeatVideoPath
		: FString(FallbackDefeatVideoPath);
}
