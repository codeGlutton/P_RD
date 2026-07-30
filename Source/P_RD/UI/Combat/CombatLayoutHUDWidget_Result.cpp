#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Kismet/GameplayStatics.h"   // 승리/패배 결과 징글 재생
#include "TimerManager.h"             // 결과 영상 시작 텀 타이머
#include "Setting/GamePlaySettings.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/RDUserWidget.h"
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

void UCombatLayoutHUDWidget::BeginCombatResultPresentation(TSharedPtr<FPresentationBarrier> Barrier, bool IsPlayerWin)
{
	mIsPlayerWin = IsPlayerWin;
	mCombatResultBarrier = MoveTemp(Barrier);
	mVictoryWorldMapLocked = false;

	// 판정 직후 바로 영상이 틀어지면 급작스러워서, 전장을 잠깐 보여주는 텀을 두고 연출을 시작한다.
	// 배리어는 이미 붙잡고 있으므로 딜레이 동안 프레임워크 진행은 멈춰 있다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			mCombatResultStartDelayTimerHandle, this, &UCombatLayoutHUDWidget::StartCombatResultCinematic,
			CombatResultStartDelaySeconds, false);
		return;
	}

	StartCombatResultCinematic();
}

/** @brief 텀(딜레이) 후 실제 결과 연출 시작 — 징글 재생 + 결과 영상 오픈. */
void UCombatLayoutHUDWidget::StartCombatResultCinematic()
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
			OpenedCinematicWidget->PlayCinematic(FOnEndCinematicAnimation::CreateUObject(this, &UCombatLayoutHUDWidget::HandleCombatResultVideoFinished));
		}
	}));
}

void UCombatLayoutHUDWidget::EnsureCombatResultWidgets()
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
		mCombatRewardWidget->OnClosed.RemoveDynamic(this, &UCombatLayoutHUDWidget::HandleCombatResultRewardConfirmed);
		mCombatRewardWidget->OnClosed.AddUniqueDynamic(this, &UCombatLayoutHUDWidget::HandleCombatResultRewardConfirmed);
		mCombatRewardWidget->BindUIModel(mCombatRewardUIModel);
	}
}

void UCombatLayoutHUDWidget::HandleCombatResultVideoFinished(UCinematicWidget* CinematicWidget)
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

void UCombatLayoutHUDWidget::HandleCombatResultOpenRequested()
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
		FSimpleDelegate::CreateUObject(this, &UCombatLayoutHUDWidget::HandleCombatResultContinueConfirmed));
	mCombatResultOverlayWidget->OpenUI();
}

void UCombatLayoutHUDWidget::HandleCombatResultRewardConfirmed()
{
	if (mCombatResultOverlayWidget != nullptr)
	{
		mCombatResultOverlayWidget->CloseUI();
	}

	mVictoryWorldMapLocked = true;

	CloseCombatResultCinematic(FSimpleDelegate::CreateWeakLambda(this, [this]() mutable
	{
		SetCombatResultViewActive(false, false);
		OpenWorldMapForNextRoom();
	}));
}

void UCombatLayoutHUDWidget::HandleCombatRewardClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
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

void UCombatLayoutHUDWidget::HandleCombatResultContinueConfirmed()
{
	if (mCombatResultOverlayWidget != nullptr)
	{
		mCombatResultOverlayWidget->CloseUI();
	}

	CloseCombatResultCinematic(FSimpleDelegate::CreateWeakLambda(this, [this]() mutable
	{
		SetCombatResultViewActive(false, true);
		
		if (mUIModel != nullptr)
		{
			mUIModel->RequestAbandonRun();
		}
	}));
}

void UCombatLayoutHUDWidget::CloseCombatResultCinematic(FSimpleDelegate Callback)
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

void UCombatLayoutHUDWidget::SetCombatResultViewActive(bool bActive, bool bRestoreCombatControls)
{
	if (bActive)
	{
		mCombatResultFlowActive = true;
		SetCombatControlsShown(false);
		return;
	}

	if (bRestoreCombatControls)
	{
		SetCombatControlsShown(true);
	}
	mCombatResultFlowActive = false;
}

FString UCombatLayoutHUDWidget::GetCombatResultVideoPath(bool IsPlayerWin) const
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

void UCombatLayoutHUDWidget::HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier)
{
	if (mUIModel == nullptr)
	{
		return;
	}

	BeginCombatResultPresentation(MoveTemp(Barrier), mUIModel->GetCombatResultUI().mIsWin);
}

/**
 * @brief 승리 뒤 다음 방을 고르도록 지도를 연다.
 *
 * @details
 * 옛 HUD 는 지도를 열고 나서 "닫아도 다시 연다" 는 잠금까지 들고 있었다.
 * 그 잠금은 지도 위젯이 방을 고를 때까지 풀리지 않는데, 여기서는 우선 여는
 * 것까지만 옮긴다 -- 잠금은 지도 흐름을 옮길 때 같이 가져온다.
 */
void UCombatLayoutHUDWidget::OpenWorldMapForNextRoom()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		return;
	}
	if (URDUserWidget* MapWidget = Cast<URDUserWidget>(
		WorldWidgetSubsystem->GetWorldWidget(EWorldWidgetType::WorldMap)))
	{
		mVictoryWorldMapLocked = true;
		MapWidget->OpenUI();
	}
}

/**
 * @brief 결과 화면이 뜬 동안 조작을 감춘다.
 *
 * 카드와 아래 단추만 감추면 된다. 판 위 표시는 결과 영상이 덮는다.
 */
void UCombatLayoutHUDWidget::SetCombatControlsShown(const bool bShown)
{
	// 결과 영상과 보상 화면은 별개 위젯으로 화면 위에 얹힌다. 겹 하나하나를
	// 접는 것보다 HUD 를 통째로 접는 편이 확실하다 -- 카드·판·아군 칸이
	// 저마다 제 조건으로 다시 켜지는 일이 없다.
	SetVisibility(bShown ? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
}
