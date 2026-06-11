#include "UI/TopMenuBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameMode/CombatGameMode.h"
#include "GameMode/RoomGameModeBase.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "UI/FrontendMapWidget.h"
#include "UI/SettingsPanelWidget.h"
#include "UI/ViewportZOrderType.h"

namespace
{
	/**
	 * @brief 현재 표시 중인 위젯인지 확인한다.
	 *
	 * @details
	 * 탑바는 루트와 일반 장식 위젯을 SelfHitTestInvisible로 바꾸되, 이미 숨겨진 위젯은 숨김 상태를 유지해야 한다.
	 */
	bool ShouldKeepWidgetVisibility(ESlateVisibility Visibility)
	{
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	}
}

/**
 * @brief 탑바가 다른 팝업 위에서 보이도록 ZOrder를 설정한다.
 *
 * @details
 * 월드맵/설정 패널도 팝업 ZOrder를 사용하므로, 탑바는 PopUp보다 한 단계 위에 둔다.
 * 대신 입력은 ApplyInputPassThrough()에서 버튼만 받게 처리해, 위에 보이더라도 아래 팝업 조작을 막지 않는다.
 */
UTopMenuBarWidget::UTopMenuBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp) + 1;
}

/**
 * @brief 위젯 구성 직후 버튼/전투 이벤트를 연결하고 입력 통과 상태를 적용한다.
 *
 * @details
 * 탑바는 직접 화면을 소유하지 않고, 버튼 입력과 전투 종료 신호를 받아 공용 월드 위젯을 여는 허브 역할을 한다.
 *
 * 왜 Construct에서 이벤트를 묶는가:
 * WBP 버튼과 전투 서브시스템이 모두 준비된 뒤에만 구독할 수 있다. 여기서 한 번 연결해 두면 방마다 별도 버튼 연결을 만들지 않아도 된다.
 */
void UTopMenuBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();
	SyncDefaultText();
	BindButtonEvents();
	BindCombatEvents();
	ApplyInputPassThrough();
}

/**
 * @brief 탑바가 사라질 때 외부 위젯과 서브시스템에 남긴 구독을 정리한다.
 */
void UTopMenuBarWidget::NativeDestruct()
{
	if (USRPGCombatSubsystem* CombatSubsystem = GetWorld() != nullptr ? GetWorld()->GetSubsystem<USRPGCombatSubsystem>() : nullptr)
	{
		CombatSubsystem->OnEndCombatUI.RemoveAll(this);
	}

	if (UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap)))
	{
		WorldMapWidget->OnCloseRequested.RemoveDynamic(this, &UTopMenuBarWidget::HandleWorldMapCloseRequested);
	}

	if (USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetToggleableWorldWidget(EWorldWidgetType::InGameSettings)))
	{
		SettingsPanelWidget->OnBackRequested.RemoveDynamic(this, &UTopMenuBarWidget::HandleSettingsBackRequested);
	}

	UnbindButtonEvents();

	Super::NativeDestruct();
}

/**
 * @brief OpenUI()로 다시 열릴 때 현재 방 요약과 입력 통과 상태를 최신화한다.
 */
void UTopMenuBarWidget::ApplyOpenUI()
{
	Super::ApplyOpenUI();
	RefreshRoomInfo();
	ApplyInputPassThrough();
}

/**
 * @brief 탑바의 장식 영역은 입력을 통과시키고 버튼만 입력을 받게 한다.
 *
 * @details
 * 인게임 탑바가 전체 화면 위젯으로 올라와도 맵 클릭, 전투 입력, 하위 팝업 입력을 막지 않게 하기 위한 설정이다.
 * 버튼은 Visible로 유지해 클릭 가능하게 두고, 나머지 표시 위젯은 SelfHitTestInvisible로 바꾼다.
 *
 * 왜 입력을 통과시키는가:
 * 탑바 WBP가 화면 전체 크기로 배치되면 보이지 않는 배경이 아래 UI의 터치를 먹을 수 있다.
 * 장식 영역은 통과시키고 버튼만 입력을 받게 해야 게임 조작과 팝업 조작이 막히지 않는다.
 */
void UTopMenuBarWidget::ApplyInputPassThrough()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (WidgetTree != nullptr)
	{
		UWidget* RootWidget = WidgetTree->RootWidget;
		WidgetTree->ForEachWidget([RootWidget](UWidget* Widget)
		{
			if (Widget == nullptr || !ShouldKeepWidgetVisibility(Widget->GetVisibility()))
			{
				return;
			}

			if (Widget == RootWidget)
			{
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				return;
			}

			if (Cast<UButton>(Widget) != nullptr)
			{
				Widget->SetVisibility(ESlateVisibility::Visible);
				return;
			}

			Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		});
	}

	ConfigureDesignerButton(MapButton);
	ConfigureDesignerButton(SettingsButton);
	ConfigureDesignerButton(DiceButton);
	ConfigureDesignerButton(SkillButton);
}

/**
 * @brief 연출용 표시 상태로 전환해 모든 입력을 통과시킨다.
 */
void UTopMenuBarWidget::ApplyDisplayOnly()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

/**
 * @brief WBP 버튼 입력을 탑바의 토글 핸들러에 연결한다.
 */
void UTopMenuBarWidget::BindButtonEvents()
{
	if (MapButton != nullptr)
	{
		MapButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleMapButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSettingsButtonClicked);
	}

	if (DiceButton != nullptr)
	{
		DiceButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleDiceButtonClicked);
	}

	if (SkillButton != nullptr)
	{
		SkillButton->OnClicked.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSkillButtonClicked);
	}
}

/**
 * @brief NativeConstruct()에서 연결한 버튼 입력을 해제한다.
 */
void UTopMenuBarWidget::UnbindButtonEvents()
{
	if (MapButton != nullptr)
	{
		MapButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleMapButtonClicked);
	}

	if (SettingsButton != nullptr)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleSettingsButtonClicked);
	}

	if (DiceButton != nullptr)
	{
		DiceButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleDiceButtonClicked);
	}

	if (SkillButton != nullptr)
	{
		SkillButton->OnClicked.RemoveDynamic(this, &UTopMenuBarWidget::HandleSkillButtonClicked);
	}
}

/**
 * @brief 모바일 입력에서 버튼이 눌림 즉시 반응하도록 버튼 입력 방식을 보정한다.
 */
void UTopMenuBarWidget::ConfigureDesignerButton(UButton* Button) const
{
	if (Button == nullptr)
	{
		return;
	}

	Button->SetVisibility(ESlateVisibility::Visible);
	Button->SetIsEnabled(true);
	Button->SetClickMethod(EButtonClickMethod::MouseDown);
	Button->SetTouchMethod(EButtonTouchMethod::Down);
	Button->SetPressMethod(EButtonPressMethod::ButtonPress);
}

/**
 * @brief WBP_TopMenuBar가 필수 버튼 바인딩을 제공하는지 로그로 확인한다.
 */
void UTopMenuBarWidget::ValidateDesignerBindings() const
{
	if (MapButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: MapButton is not connected."));
	}

	if (SettingsButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: SettingsButton is not connected."));
	}

	if (DiceButton == nullptr)
	{
		UE_LOG(LogRD, Verbose, TEXT("TopMenuBarWidget: DiceButton is not connected."));
	}

	if (SkillButton == nullptr)
	{
		UE_LOG(LogRD, Verbose, TEXT("TopMenuBarWidget: SkillButton is not connected."));
	}
}

/**
 * @brief 기본 라벨과 현재 방 요약 문구를 채운다.
 *
 * @details
 * SKILL 버튼은 전투방에서 임시 승리 트리거로 쓰일 수 있으므로 전투방일 때만 WIN 문구를 보여준다.
 */
void UTopMenuBarWidget::SyncDefaultText() const
{
	if (MapButtonText != nullptr)
	{
		MapButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "MapButtonText", "MAP"));
	}

	if (SettingsButtonText != nullptr)
	{
		SettingsButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "SettingsButtonText", "SET"));
	}

	if (DiceButtonText != nullptr)
	{
		DiceButtonText->SetText(NSLOCTEXT("TopMenuBarWidget", "DiceButtonText", "DICE 0"));
	}

	if (SkillButtonText != nullptr)
	{
		const bool bIsCombatRoom = GetWorld() != nullptr && GetWorld()->GetAuthGameMode<ACombatGameMode>() != nullptr;
		SkillButtonText->SetText(bIsCombatRoom
			? NSLOCTEXT("TopMenuBarWidget", "DebugVictoryButtonText", "WIN")
			: NSLOCTEXT("TopMenuBarWidget", "SkillButtonText", "SKILL 0"));
	}

	if (TitleTextBlock != nullptr)
	{
		TitleTextBlock->SetText(NSLOCTEXT("TopMenuBarWidget", "TitleText", "ROOM"));
	}

	if (SummaryTextBlock != nullptr)
	{
		SummaryTextBlock->SetText(FText::GetEmpty());
	}

	RefreshRoomInfo();
}

/**
 * @brief RoomGameMode가 제공하는 현재 런 정보를 탑바 제목/요약으로 변환한다.
 */
void UTopMenuBarWidget::RefreshRoomInfo() const
{
	ARoomGameModeBase* GameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr;
	FFrontendRunControlView RunControlView;
	if (GameMode == nullptr || !GameMode->GetRunControlView(OUT RunControlView))
	{
		if (TitleTextBlock != nullptr)
		{
			TitleTextBlock->SetText(NSLOCTEXT("TopMenuBarWidget", "TitleTextFallback", "ROOM"));
		}
		if (SummaryTextBlock != nullptr)
		{
			SummaryTextBlock->SetText(FText::GetEmpty());
		}
		return;
	}

	if (TitleTextBlock != nullptr)
	{
		TitleTextBlock->SetText(FText::Format(
			NSLOCTEXT("TopMenuBarWidget", "RoomTitleFormat", "ROOM {0}-{1}"),
			FText::AsNumber(RunControlView.mRow + 1),
			FText::AsNumber(RunControlView.mColumn + 1)));
	}

	if (SummaryTextBlock != nullptr)
	{
		SummaryTextBlock->SetText(FText::Format(
			NSLOCTEXT("TopMenuBarWidget", "RoomSummaryFormat", "Lv {0} | Difficulty {1}"),
			FText::AsNumber(RunControlView.mPlayerLevel),
			FText::AsNumber(RunControlView.mDifficulty)));
	}
}

/**
 * @brief 전투 종료 UI 이벤트를 구독한다.
 *
 * @details
 * 전투 결과 판단은 SRPGCombatSubsystem의 책임으로 두고, 탑바는 플레이어 승리 결과만 받아 월드맵 표시 흐름을 시작한다.
 */
void UTopMenuBarWidget::BindCombatEvents()
{
	USRPGCombatSubsystem* CombatSubsystem = GetWorld() != nullptr ? GetWorld()->GetSubsystem<USRPGCombatSubsystem>() : nullptr;
	if (CombatSubsystem == nullptr)
	{
		return;
	}

	CombatSubsystem->OnEndCombatUI.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result)
	{
		HandleEndCombatUI(MoveTemp(Barrier), Result);
	});
}

/**
 * @brief 월드 서브시스템에 등록된 월드 위젯을 URDUserWidget으로 가져온다.
 */
URDUserWidget* UTopMenuBarWidget::GetToggleableWorldWidget(EWorldWidgetType WorldWidgetType) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	const UWorldWidgetSubsystem* WorldWidgetSubsystem = World->GetSubsystem<UWorldWidgetSubsystem>();
	if (WorldWidgetSubsystem == nullptr)
	{
		return nullptr;
	}

	return Cast<URDUserWidget>(WorldWidgetSubsystem->GetWorldWidget(WorldWidgetType));
}

/**
 * @brief 지정한 월드 위젯을 공통 CloseUI() 경로로 닫는다.
 */
void UTopMenuBarWidget::CloseWorldWidget(EWorldWidgetType WorldWidgetType) const
{
	if (URDUserWidget* Widget = GetToggleableWorldWidget(WorldWidgetType))
	{
		Widget->CloseUI();
	}
}

/**
 * @brief 일반 맵 조회와 승리 후 다음 방 선택 맵을 구분해 월드맵을 토글한다.
 *
 * @details
 * 사용자가 MAP 버튼으로 연 지도는 조회용이므로 방 선택을 막는다.
 * 승리 후 지도 잠금 상태에서는 다음 방 선택을 유지해야 하므로 닫기 대신 복원 흐름으로 보낸다.
 *
 * 왜 탑바가 지도 모드를 정하는가:
 * 같은 WorldMap 위젯이라도 MAP 버튼에서 열렸는지, 전투 승리 후 열렸는지에 따라 선택 가능 여부가 달라진다.
 * 열린 경로를 알고 있는 탑바가 SetRoomSelectionEnabled()를 정해야 위젯 내부가 게임 흐름을 추측하지 않는다.
 */
void UTopMenuBarWidget::ToggleWorldMap()
{
	RefreshRoomInfo();

	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: WorldMap widget is not configured."));
		return;
	}

	if (mVictoryWorldMapLocked)
	{
		CloseSettingsPanelAndRestoreVictoryWorldMap();
		return;
	}

	if (WorldMapWidget->IsOpened())
	{
		WorldMapWidget->CloseUI();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(false);
	WorldMapWidget->ClearMapStatusOverride();
	WorldMapWidget->OpenUI();
	WorldMapWidget->RefreshMap();
	ApplyInputPassThrough();
}

/**
 * @brief 인게임 설정 패널을 토글한다.
 *
 * @details
 * 설정 패널은 월드맵과 동시에 열지 않는다. 승리 후 지도 선택이 잠겨 있으면 설정을 닫은 뒤 월드맵을 다시 띄운다.
 *
 * 왜 설정 패널 상태를 여기서 초기화하는가:
 * 인게임 SET 버튼에서 열린 설정은 항상 런 액션 영역을 기준으로 시작해야 한다.
 * TopMenuBar가 모드와 상태 문구를 정리해주면 SettingsPanelWidget은 어느 화면에서 열렸는지 직접 추측하지 않아도 된다.
 */
void UTopMenuBarWidget::ToggleSettingsPanel()
{
	RefreshRoomInfo();

	USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetToggleableWorldWidget(EWorldWidgetType::InGameSettings));
	if (SettingsPanelWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("TopMenuBarWidget: InGameSettings widget is not configured."));
		return;
	}

	if (SettingsPanelWidget->IsOpened())
	{
		if (mVictoryWorldMapLocked)
		{
			CloseSettingsPanelAndRestoreVictoryWorldMap();
			return;
		}

		SettingsPanelWidget->CloseUI();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::WorldMap);
	SettingsPanelWidget->OnBackRequested.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleSettingsBackRequested);
	SettingsPanelWidget->OpenUI();
	SettingsPanelWidget->SetPanelMode(ESettingsPanelMode::InGame);
	SettingsPanelWidget->RefreshPanelState(false, false);
	SettingsPanelWidget->HideAbandonConfirm();
	SettingsPanelWidget->SetStatusText(FText::GetEmpty());
	ApplyInputPassThrough();
}

/**
 * @brief MAP 버튼 클릭을 월드맵 토글로 연결한다.
 */
void UTopMenuBarWidget::HandleMapButtonClicked()
{
	ToggleWorldMap();
}

/**
 * @brief SET 버튼 클릭을 설정 패널 토글로 연결한다.
 */
void UTopMenuBarWidget::HandleSettingsButtonClicked()
{
	ToggleSettingsPanel();
}

/**
 * @brief 아직 연결되지 않은 DICE 팝업 입력을 로그로 남긴다.
 */
void UTopMenuBarWidget::HandleDiceButtonClicked()
{
	UE_LOG(LogRD, Log, TEXT("TopMenuBarWidget: DICE button clicked. Dice popup is not configured on this branch."));
}

/**
 * @brief 아직 연결되지 않은 SKILL 팝업 또는 임시 승리 입력을 로그로 남긴다.
 */
void UTopMenuBarWidget::HandleSkillButtonClicked()
{
	UE_LOG(LogRD, Log, TEXT("TopMenuBarWidget: SKILL button clicked. Skill popup is not configured on this branch."));
}

/**
 * @brief 플레이어 승리 결과를 다음 방 선택 월드맵 표시로 연결한다.
 */
void UTopMenuBarWidget::HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier, ESRPGCombatResult Result)
{
	if (Result != ESRPGCombatResult::PlayerWin)
	{
		return;
	}

	mVictoryWorldMapLocked = true;
	OpenWorldMapAfterPlayerWin(MoveTemp(Barrier));
}

/**
 * @brief 승리 후 다음 방 선택이 가능한 상태로 월드맵을 연다.
 *
 * @details
 * OpenUI 완료 후 barrier를 해제해 전투 종료 연출과 다음 방 선택 UI의 경계가 끊기지 않게 한다.
 *
 * 왜 즉시 RefreshMap()도 호출하는가:
 * OpenUI 애니메이션이 있는 경우에도 위젯 내용은 먼저 채워져 있어야 한다.
 * 콜백에서도 한 번 더 갱신해, 열리는 중 선택 상태가 바뀌어도 표시가 최신 상태가 된다.
 *
 * 왜 WeakLambda 대상을 WorldMapWidget으로 두는가:
 * 완료 콜백에서 실제로 다시 접근하는 UObject는 탑바가 아니라 월드맵 위젯이다.
 * 따라서 월드맵 위젯이 사라진 뒤 콜백이 실행되지 않도록 OpenUI 대상 위젯을 약한 참조 검사 대상으로 둔다.
 */
void UTopMenuBarWidget::OpenWorldMapAfterPlayerWin(TSharedPtr<FPresentationBarrier> Barrier)
{
	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UTopMenuBarWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(true);
	WorldMapWidget->SetMapStatusOverride(NSLOCTEXT("TopMenuBarWidget", "VictoryMapStatus", "승리했습니다!"));
	WorldMapWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(WorldMapWidget, [Barrier](UUserWidget* OpenedWidget) mutable
	{
		if (UFrontendMapWidget* OpenedWorldMapWidget = Cast<UFrontendMapWidget>(OpenedWidget))
		{
			OpenedWorldMapWidget->RefreshMap();
		}
		Barrier.Reset();
	}));
	WorldMapWidget->RefreshMap();
	ApplyInputPassThrough();
}

/**
 * @brief 승리 후 지도 잠금이 유지되는 동안 월드맵을 다시 연다.
 */
void UTopMenuBarWidget::RestoreVictoryWorldMap()
{
	if (!mVictoryWorldMapLocked)
	{
		return;
	}

	OpenWorldMapAfterPlayerWin(TSharedPtr<FPresentationBarrier>());
}

/**
 * @brief 설정 패널이 열려 있으면 닫힘 완료 뒤 승리 후 월드맵을 복원한다.
 */
void UTopMenuBarWidget::CloseSettingsPanelAndRestoreVictoryWorldMap()
{
	if (URDUserWidget* SettingsPanelWidget = GetToggleableWorldWidget(EWorldWidgetType::InGameSettings);
		SettingsPanelWidget != nullptr && SettingsPanelWidget->IsOpened())
	{
		SettingsPanelWidget->CloseUI(FOnEndUICloseAnimation::CreateWeakLambda(this, [this](UUserWidget*)
		{
			RestoreVictoryWorldMap();
		}));
		return;
	}

	RestoreVictoryWorldMap();
}

/**
 * @brief 월드맵 닫기 요청을 현재 지도 잠금 상태에 맞게 처리한다.
 */
void UTopMenuBarWidget::HandleWorldMapCloseRequested()
{
	if (mVictoryWorldMapLocked)
	{
		RestoreVictoryWorldMap();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::WorldMap);
}

/**
 * @brief 설정 패널의 Back 요청을 현재 지도 잠금 상태에 맞게 처리한다.
 */
void UTopMenuBarWidget::HandleSettingsBackRequested()
{
	if (mVictoryWorldMapLocked)
	{
		CloseSettingsPanelAndRestoreVictoryWorldMap();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
}
