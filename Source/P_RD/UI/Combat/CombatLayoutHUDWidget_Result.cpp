#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
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
#include "UI/Reward/RewardSettlementWidgetBase.h"

namespace
{
	const TCHAR* const FallbackVictoryVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Victory_01.mp4");
	const TCHAR* const FallbackDefeatVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Defeat_01.mp4");
	constexpr int32 CombatResultVideoZOrder = -20;
	/** @brief 패배 판정 후 결과 영상 시작까지의 텀(전장을 잠깐 보여주는 시간). */
	constexpr float CombatResultStartDelaySeconds = 1.2f;
}

void UCombatLayoutHUDWidget::BeginCombatResultPresentation(TSharedPtr<FPresentationBarrier> Barrier, bool IsPlayerWin)
{
	mIsPlayerWin = IsPlayerWin;
	mCombatResultBarrier = MoveTemp(Barrier);
	mVictoryWorldMapLocked = false;

	// 승리와 패배 모두 결과 영상을 사용하지 않는다. 결과 위젯을 먼저 준비한 뒤
	// 배리어를 놓으면 전투 모델이 같은 호출 흐름에서 결과 데이터를 밀고
	// OpenRequested를 방송한다. 패배는 짧은 징글만 재생한다.
	EnsureCombatResultWidgets();
	SetCombatResultViewActive(true);
	if (mIsPlayerWin == false && mDefeatJingleSound != nullptr)
	{
		UGameplayStatics::PlaySound2D(this, mDefeatJingleSound);
	}
	mCombatResultBarrier.Reset();
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
	if (mCombatResultOverlayWidget == nullptr)
	{
		// 생성자에서 하드 레퍼런스로 든 클래스를 우선 쓴다. 문자열 로드는
		// 테스트 픽스처 등 클래스가 비어 있는 환경의 폴백이다.
		UClass* DefeatWidgetClass = mDefeatWidgetClass.Get();
		if (DefeatWidgetClass == nullptr)
		{
			static const TCHAR* DefeatWidgetClassPath =
				TEXT("/Game/UI/CombatResult/WBP_CombatDefeat.WBP_CombatDefeat_C");
			DefeatWidgetClass = LoadClass<UCombatResultOverlayWidget>(nullptr, DefeatWidgetClassPath);
			if (DefeatWidgetClass == nullptr)
			{
				UE_LOG(LogRD, Warning, TEXT("Combat defeat WBP is unavailable: %s"), DefeatWidgetClassPath);
				DefeatWidgetClass = UCombatResultOverlayWidget::StaticClass();
			}
		}
		mCombatResultOverlayWidget = CreateWidget<UCombatResultOverlayWidget>(GetOwningPlayer(), DefeatWidgetClass);
	}

	if (mCombatRewardWidget == nullptr && mRewardWidgetClass != nullptr)
	{
		mCombatRewardWidget = CreateWidget<URewardSettlementWidgetBase>(GetOwningPlayer(), mRewardWidgetClass);
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

	mCombatResultOverlayWidget->ShowDefeatResult(
		mUIModel->GetCombatResultUI(),
		FSimpleDelegate::CreateUObject(this, &UCombatLayoutHUDWidget::HandleCombatResultContinueConfirmed),
		FSimpleDelegate::CreateUObject(this, &UCombatLayoutHUDWidget::HandleCombatResultRetryConfirmed));
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

void UCombatLayoutHUDWidget::HandleCombatRewardClaimConfirmed(ERewardClaimKind ClaimKind, int32 ChoiceIndex)
{
	// 지급 성공이 확인된 뒤에만 효과음을 낸다. 실패한 claim 요청은 행 상태와
	// 소리를 바꾸지 않아 사용자가 실제 지급 여부를 오해하지 않게 한다.
	if (ClaimKind == ERewardClaimKind::Exp && mExpGainSound != nullptr)
	{
		UGameplayStatics::PlaySound2D(this, mExpGainSound);
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

void UCombatLayoutHUDWidget::HandleCombatResultRetryConfirmed()
{
	if (mCombatResultOverlayWidget != nullptr)
	{
		mCombatResultOverlayWidget->CloseUI();
	}

	SetCombatResultViewActive(false, false);
	if (mUIModel != nullptr)
	{
		mUIModel->RequestRetryCombat();
	}
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
 * 보상 확인 뒤에는 조회용 지도가 아니라 다음 방 선택용 지도를 열어야 한다.
 * 그래서 선택 모드와 최신 런 데이터를 먼저 적용하고, 다음 방으로 실제
 * 전환되기 전에는 BACK으로 닫아 전투가 끝난 화면으로 돌아갈 수 없게 한다.
 */
void UCombatLayoutHUDWidget::OpenWorldMapForNextRoom()
{
	UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld() != nullptr
		? GetWorld()->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatLayoutHUDWidget: WorldWidgetSubsystem is unavailable after victory."));
		return;
	}

	UFrontendMapWidget* MapWidget = Cast<UFrontendMapWidget>(
		WorldWidgetSubsystem->GetWorldWidget(EWorldWidgetType::WorldMap));
	if (MapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatLayoutHUDWidget: WorldMap widget is not configured."));
		return;
	}

	mVictoryWorldMapLocked = true;
	MapWidget->OnCloseRequested.AddUniqueDynamic(
		this, &UCombatLayoutHUDWidget::HandleWorldMapCloseRequested);
	MapWidget->SetRoomSelectionEnabled(true);
	MapWidget->ClearMapStatusOverride();
	MapWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(MapWidget, [](UUserWidget* OpenedWidget)
	{
		if (UFrontendMapWidget* OpenedMapWidget = Cast<UFrontendMapWidget>(OpenedWidget))
		{
			OpenedMapWidget->RefreshMap();
		}
	}));
	// OpenUI가 이미 열린 위젯에서 즉시 반환하는 경우도 있으므로 여기서도
	// 한 번 갱신한다. 갱신은 선택/ENTER 활성 상태까지 함께 다시 계산한다.
	MapWidget->RefreshMap();
}

/**
 * @brief 전투 HUD의 MAP 단추로 현재 런 지도를 조회한다.
 *
 * @details
 * 조회 지도는 방을 고를 수 없으며, 열려 있는 동안 HUD와 게임 입력을 막는다.
 * 전투판 위에 전체 화면 위젯만 얹고 입력 모드를 그대로 두면 키보드 단축키나
 * 판 클릭이 뒤로 새어 행동을 확정할 수 있어 UIOnly로 전환한다.
 */
void UCombatLayoutHUDWidget::OpenWorldMapForCombatReview()
{
	if (mCombatResultFlowActive || mVictoryWorldMapLocked)
	{
		return;
	}

	UWorld* World = GetWorld();
	UWorldWidgetSubsystem* WorldWidgetSubsystem = World != nullptr
		? World->GetSubsystem<UWorldWidgetSubsystem>() : nullptr;
	if (WorldWidgetSubsystem == nullptr)
	{
		UE_LOG(LogRD, Warning,
			TEXT("CombatLayoutHUDWidget: WorldWidgetSubsystem is unavailable for combat map review."));
		return;
	}

	UFrontendMapWidget* MapWidget = WorldWidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
		EWorldWidgetType::WorldMap);
	if (MapWidget == nullptr)
	{
		// 정상 방 진입에서는 RoomGameModeBase가 미리 만든다. 자동화/직접 로드처럼
		// 초기화 순서가 다른 경로에서도 MAP 단추가 조용히 죽지 않게 한 번 보완한다.
		WorldWidgetSubsystem->InitWorldWidget(EWorldWidgetType::WorldMap);
		MapWidget = WorldWidgetSubsystem->GetWorldWidget<UFrontendMapWidget>(
			EWorldWidgetType::WorldMap);
	}
	if (MapWidget == nullptr && mWorldMapWidgetClass != nullptr)
	{
		// 서브시스템 생성은 OwningPlayer를 요구한다. 플레이어 컨트롤러보다 HUD가
		// 먼저 구성되는 테스트/직접 레벨 로드에서는 World를 소유자로 직접 만든다.
		// 정상 방에서는 위 분기에서 공용 인스턴스를 얻으므로 중복 생성되지 않는다.
		MapWidget = CreateWidget<UFrontendMapWidget>(
			World, mWorldMapWidgetClass);
	}
	if (MapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning,
			TEXT("CombatLayoutHUDWidget: WorldMap widget is not configured for combat map review."));
		return;
	}

	// 다른 전투 팝업과 조준 상태를 남겨 둔 채 지도를 열지 않는다.
	SetMercenaryPanelShown(false);
	HideDetailOverlay(/*bNotifyGameplay=*/true);
	if (mUIModel != nullptr && IsAiming())
	{
		mUIModel->RequestCancel();
	}

	mCombatReviewWorldMap = MapWidget;
	mCombatReviewWorldMapOpen = true;
	MapWidget->OnCloseRequested.AddUniqueDynamic(
		this, &UCombatLayoutHUDWidget::HandleWorldMapCloseRequested);
	MapWidget->SetRoomSelectionEnabled(false);
	MapWidget->ClearMapStatusOverride();

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		mShowMouseCursorBeforeCombatReviewMap = PlayerController->bShowMouseCursor;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MapWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}

	SetCombatControlsShown(false);
	MapWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(
		MapWidget, [](UUserWidget* OpenedWidget)
		{
			if (UFrontendMapWidget* OpenedMapWidget =
				Cast<UFrontendMapWidget>(OpenedWidget))
			{
				OpenedMapWidget->RefreshMap();
			}
		}));
	MapWidget->RefreshMap();
}

/** @brief 조회용 지도를 닫는다. 실제 입력 복원은 닫기 완료 뒤에만 수행한다. */
void UCombatLayoutHUDWidget::CloseWorldMapForCombatReview()
{
	if (mCombatReviewWorldMapOpen == false)
	{
		return;
	}

	mCombatReviewWorldMapOpen = false;
	UFrontendMapWidget* MapWidget = mCombatReviewWorldMap;
	if (MapWidget == nullptr)
	{
		RestoreCombatInputAfterWorldMap();
		return;
	}

	MapWidget->CloseUI(FOnEndUICloseAnimation::CreateWeakLambda(
		this, [this](UUserWidget*)
		{
			RestoreCombatInputAfterWorldMap();
		}));
}

/** @brief 조회 지도 뒤에 숨긴 HUD와 전투 입력 모드를 원래 방 기본값으로 되돌린다. */
void UCombatLayoutHUDWidget::RestoreCombatInputAfterWorldMap()
{
	if (mCombatReviewWorldMap != nullptr)
	{
		mCombatReviewWorldMap->OnCloseRequested.RemoveDynamic(
			this, &UCombatLayoutHUDWidget::HandleWorldMapCloseRequested);
	}
	mCombatReviewWorldMap = nullptr;

	// 지도를 보는 사이 결과 흐름이 시작됐다면 결과 화면이 숨긴 HUD를 되살리면
	// 안 된다. 입력 모드만 전투 방 기본값으로 돌려 다음 화면이 이어받게 한다.
	if (mCombatResultFlowActive == false)
	{
		SetCombatControlsShown(true);
		// 지도를 열며 조준과 상세를 다 걷었다. 돌아온 화면에 카드까지 접혀
		// 있으면 "지도 갔다 오니 조작이 사라졌다"로 읽힌다.
		SetCommandsShown(true);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(
			mShowMouseCursorBeforeCombatReviewMap);
	}
}

/** @brief MAP 메뉴는 같은 조회 지도를 열고 닫는 토글이다. */
void UCombatLayoutHUDWidget::HandleWorldMapMenuClicked()
{
	if (mCombatReviewWorldMapOpen)
	{
		CloseWorldMapForCombatReview();
		return;
	}
	OpenWorldMapForCombatReview();
}

void UCombatLayoutHUDWidget::HandleWorldMapCloseRequested()
{
	if (mVictoryWorldMapLocked)
	{
		OpenWorldMapForNextRoom();
		return;
	}
	CloseWorldMapForCombatReview();
}

/**
 * @brief 결과 화면이 뜬 동안 조작을 감춘다.
 *
 * 카드와 아래 단추만 감추면 된다. 판 위 표시는 결과 영상이 덮는다.
 */
void UCombatLayoutHUDWidget::SetCombatControlsShown(const bool bShown)
{
	// 상세 패널은 뷰포트에 따로 얹혀 있어 HUD 를 접어도 같이 안 접힌다.
	// 결과 화면 위에 남으면 안 되니 여기서 닫는다.
	if (bShown == false)
	{
		HideDetailOverlay(/*bNotifyGameplay=*/false);
	}

	// 결과 영상과 보상 화면은 별개 위젯으로 화면 위에 얹힌다. 겹 하나하나를
	// 접는 것보다 HUD 를 통째로 접는 편이 확실하다 -- 카드·판·아군 칸이
	// 저마다 제 조건으로 다시 켜지는 일이 없다.
	//
	// 되살릴 때는 **Visible** 이어야 한다. 이 HUD 는 화면 전체로 눌림을 받아
	// 판 탭(카드 여닫기·적 살펴보기)을 처리한다(OpenUI 주석 참고).
	// SelfHitTestInvisible 로 되돌리면 자식 버튼만 눌리고 판 탭이 죽는다 --
	// 전투 중 지도를 닫고 돌아오면 빈 땅 탭이 전부 무시되던 원인이다.
	SetVisibility(bShown ? ESlateVisibility::Visible
		: ESlateVisibility::Collapsed);
}
