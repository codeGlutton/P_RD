#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "GameMode/CombatGameMode.h"
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

void UCombatTileMapHUDWidget::BeginCombatResultPresentation(TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result)
{
	mCombatResult = Result;
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
	USoundBase* ResultJingle = mCombatResult == ESRPGCombatResult::PlayerWin ? mVictoryJingleSound.Get() : mDefeatJingleSound.Get();
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
	mCombatResultCinematicWidget->SetCinematicVideoPath(GetCombatResultVideoPath(mCombatResult));
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

	if (mCombatRewardUIModel == nullptr)
	{
		mCombatRewardUIModel = NewObject<URewardUIModel>(this);
	}
	if (mCombatRewardUIModel != nullptr)
	{
		mCombatRewardUIModel->OnRewardClaimRequested.RemoveDynamic(this, &UCombatTileMapHUDWidget::HandleCombatRewardClaimRequested);
		mCombatRewardUIModel->OnRewardClaimRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleCombatRewardClaimRequested);
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
		mCombatResultBarrier.Reset();
		return;
	}

	if (mCombatResult == ESRPGCombatResult::PlayerWin)
	{
		FRewardUI RewardUI;
		TArray<FRewardChoiceUI> RewardChoices;
		if (ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
		{
			RewardUI = CombatGameMode->MakeCombatRewardUI();
			RewardChoices = CombatGameMode->MakeCombatRewardChoicesUI();
		}

		if (mCombatRewardWidget != nullptr && mCombatRewardUIModel != nullptr)
		{
			mCombatRewardWidget->BindUIModel(mCombatRewardUIModel);
			mCombatRewardUIModel->SetReward(RewardUI);
			mCombatRewardUIModel->SetRewardChoices(RewardChoices);
			mCombatRewardWidget->OpenUI();
			return;
		}

		mCombatResultOverlayWidget->ShowVictoryReward(
			RewardUI,
			FSimpleDelegate::CreateUObject(this, &UCombatTileMapHUDWidget::HandleCombatResultRewardConfirmed));
		mCombatResultOverlayWidget->OpenUI();
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

	if (ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
	{
		CombatGameMode->ClaimCombatReward();
	}

	TSharedPtr<FPresentationBarrier> Barrier = mCombatResultBarrier;
	mCombatResultBarrier.Reset();
	mVictoryWorldMapLocked = true;

	CloseCombatResultCinematic(FSimpleDelegate::CreateWeakLambda(this, [this, Barrier]() mutable
	{
		SetCombatResultViewActive(false, false);
		OpenWorldMapAfterPlayerWin(MoveTemp(Barrier));
	}));
}

void UCombatTileMapHUDWidget::HandleCombatRewardClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr;
	if (CombatGameMode == nullptr)
	{
		return;
	}

	// 지급 성공 판정은 게임플레이가 소유한다. 실제로 지급된 경우에만 UI가 그 행을 제거하도록 통지한다.
	const bool bGranted = CombatGameMode->ClaimCombatReward(ClaimKind, ChoiceIndex);
	if (bGranted && mCombatRewardWidget != nullptr)
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

	TSharedPtr<FPresentationBarrier> Barrier = mCombatResultBarrier;
	mCombatResultBarrier.Reset();

	CloseCombatResultCinematic(FSimpleDelegate::CreateWeakLambda(this, [this, Barrier]() mutable
	{
		SetCombatResultViewActive(false, true);
		Barrier.Reset();

		if (ACombatGameMode* CombatGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ACombatGameMode>() : nullptr)
		{
			CombatGameMode->AbandonRunFromRoom();
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
		SetRoomNameClusterVisible(false);
		return;
	}

	SetRoomNameClusterVisible(true);
	if (bRestoreCombatControls)
	{
		SetCombatPlayControlsVisible(true);
	}
	mCombatResultFlowActive = false;
}

void UCombatTileMapHUDWidget::SetRoomNameClusterVisible(bool bVisible)
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	static const FName RoomNameWidgets[] = {
		FName(TEXT("R_room_name_room_name_7")),
		FName(TEXT("HUD_M_room_name_text"))
	};

	for (const FName WidgetName : RoomNameWidgets)
	{
		UWidget* Widget = WidgetTree->FindWidget(WidgetName);
		if (Widget == nullptr)
		{
			continue;
		}

		if (bVisible)
		{
			if (ESlateVisibility* PreviousVisibility = mCombatResultRoomNameVisibilities.Find(WidgetName))
			{
				Widget->SetVisibility(*PreviousVisibility);
			}
			else
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			continue;
		}

		if (mCombatResultRoomNameVisibilities.Contains(WidgetName) == false)
		{
			mCombatResultRoomNameVisibilities.Add(WidgetName, Widget->GetVisibility());
		}
		Widget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (bVisible)
	{
		mCombatResultRoomNameVisibilities.Reset();
	}
}

FString UCombatTileMapHUDWidget::GetCombatResultVideoPath(ESRPGCombatResult Result) const
{
	const UGamePlaySettings* GamePlaySettings = GetDefault<UGamePlaySettings>();
	if (Result == ESRPGCombatResult::PlayerWin)
	{
		return GamePlaySettings != nullptr && GamePlaySettings->mCombatVictoryVideoPath.IsEmpty() == false
			? GamePlaySettings->mCombatVictoryVideoPath
			: FString(FallbackVictoryVideoPath);
	}

	return GamePlaySettings != nullptr && GamePlaySettings->mCombatDefeatVideoPath.IsEmpty() == false
		? GamePlaySettings->mCombatDefeatVideoPath
		: FString(FallbackDefeatVideoPath);
}
