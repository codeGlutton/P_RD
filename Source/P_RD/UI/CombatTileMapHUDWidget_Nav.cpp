#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"   // 지도 열기 사운드 재생
#include "UI/IndexedButtonWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "UI/FrontendMapWidget.h"
#include "UI/SettingsPanelWidget.h"

/*
 * 패널 내비게이션 + 승리 후 월드맵 흐름 (레거시 탑바에서 이관).
 *
 * 원래 UTopMenuBarWidget이 소유하던 두 책임을 전투 HUD로 옮긴 파일이다:
 * 1) 플로팅 패널(MAP/SET/SKILL) 토글 — 세 패널은 같은 "현재 조작 중인 팝업" 자리를
 *    공유하므로 새 패널을 열기 전에 나머지를 닫는다(상호배타).
 * 2) 승리 후 다음 방 선택 월드맵 강제 흐름 — 전투 승리 시 월드맵을 방 선택 가능 상태로 열고,
 *    사용자가 지도를 닫거나 설정을 열었다 닫아도 다음 방을 고를 때까지 지도를 복원한다.
 *
 * 스킨 HUD가 유일한 전투 화면이 되면서 concept 내비 버튼(HandleNav*Clicked)이 이 로직을
 * 직접 호출한다(탑바 경유 위임 제거).
 */

/**
 * @brief 전투 모델의 전투 종료 이벤트를 구독해 승리 후 월드맵 흐름을 시작한다.
 *
 * @details
 * BindCombatUIModel 시점(BeginRoom, InitCombat 이후)에 호출되므로 전투 모델이 항상 존재한다.
 * 재바인딩 시 이중 구독을 막기 위해 기존 구독을 먼저 해제한다.
 * 새 전투 시작 시 승리 잠금도 명시적으로 초기화한다(이전 방의 잠금이 승계되지 않게).
 */
void UCombatTileMapHUDWidget::BindVictoryFlowEvents()
{
	mVictoryWorldMapLocked = false;
	mCombatResultFlowActive = false;

	if (mCombatUIModel == nullptr)
	{
		return;
	}

	mCombatUIModel->OnEndCombat.RemoveAll(this);
	mCombatUIModel->OnEndCombat.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier)
		{
			HandleEndCombatUI(MoveTemp(Barrier));
		});

	// 라운드 시작 배너: 게임모드가 데이터(mRound) 갱신 후 재방송하는 OnBeginAnyRoundUI를 구독한다.
	// 배리어를 붙잡고 배너를 재생 → 배너 종료 시(FinishTurnChangeIntro) 배리어를 놓아 그 라운드 첫 턴이 진행된다.
	// (프레임워크가 아직 OnBeginAnyRoundUI를 방송하지 않으면 이 핸들러는 호출되지 않음 = 기존 동작 유지)
	mCombatUIModel->OnBeginAnyRound.RemoveAll(this);
	mCombatUIModel->OnBeginAnyRound.AddWeakLambda(this, [this](TSharedPtr<FPresentationBarrier> Barrier)
		{
			mRoundChangeBarrier.Reset();
			mRoundChangeBarrier = MoveTemp(Barrier);
			// 배너를 못 틀면(프레임 에셋 없음 등) 배리어를 즉시 놓아 라운드/턴이 멈추지 않게 한다.
			if (PlayTurnChangeIntro() == false)
			{
				mRoundChangeBarrier.Reset();
			}
		});
}

/**
 * @brief 월드 서브시스템에 등록된 월드 위젯을 URDUserWidget으로 가져온다.
 */
URDUserWidget* UCombatTileMapHUDWidget::GetToggleableWorldWidget(EWorldWidgetType WorldWidgetType) const
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
void UCombatTileMapHUDWidget::CloseWorldWidget(EWorldWidgetType WorldWidgetType) const
{
	if (URDUserWidget* Widget = GetToggleableWorldWidget(WorldWidgetType))
	{
		Widget->CloseUI();
	}
}

/**
 * @brief 플로팅 패널을 하나만 남기고 닫는다.
 *
 * @details
 * 월드맵, 설정, 스킬 패널은 서로 겹쳐 열리는 화면이 아니다.
 * 새 패널을 열기 전에 나머지를 닫아야 Back/Close 입력과 터치 대상이 명확하다.
 */
void UCombatTileMapHUDWidget::CloseFloatingPanels(EWorldWidgetType ExceptWorldWidgetType) const
{
	constexpr EWorldWidgetType FloatingPanels[] = {
		EWorldWidgetType::WorldMap,
		EWorldWidgetType::InGameSettings,
		EWorldWidgetType::SkillPanel
	};

	for (const EWorldWidgetType FloatingPanel : FloatingPanels)
	{
		if (FloatingPanel != ExceptWorldWidgetType)
		{
			CloseWorldWidget(FloatingPanel);
		}
	}
}

/**
 * @brief 일반 맵 조회와 승리 후 다음 방 선택 맵을 구분해 월드맵을 토글한다.
 *
 * @details
 * 사용자가 MAP 버튼으로 연 지도는 조회용이므로 방 선택을 막는다.
 * 승리 후 지도 잠금 상태에서는 다음 방 선택을 유지해야 하므로 닫기 대신 복원 흐름으로 보낸다.
 * 같은 WorldMap 위젯이라도 열린 경로에 따라 선택 가능 여부가 달라지므로, 여는 쪽이
 * SetRoomSelectionEnabled()를 정해야 위젯 내부가 게임 흐름을 추측하지 않는다.
 */
void UCombatTileMapHUDWidget::ToggleWorldMap()
{
	if (mCombatResultFlowActive)
	{
		return;
	}

	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatTileMapHUDWidget: WorldMap widget is not configured."));
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
		SetCombatPlayControlsVisible(true);   // 지도 닫힘 → 전투 컨트롤 복원.
		return;
	}

	// 여기부터는 지도가 실제로 '열리는' 경로 — 책장 넘김 사운드를 1회 재생한다(닫을 땐 안 냄).
	if (mMapOpenSound != nullptr)
	{
		UGameplayStatics::PlaySound2D(this, mMapOpenSound);
	}

	CloseFloatingPanels(EWorldWidgetType::WorldMap);
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(false);
	WorldMapWidget->ClearMapStatusOverride();
	WorldMapWidget->OpenUI();
	WorldMapWidget->RefreshMap();
	SetCombatPlayControlsVisible(false);   // 지도 열림 → 탑바만 남기고 전투 컨트롤 숨김(풀스크린 지도 뷰).
}

/**
 * @brief 인게임 설정 패널을 토글한다.
 *
 * @details
 * 설정 패널은 월드맵과 동시에 열지 않는다. 승리 후 지도 잠금 중이면 설정을 닫은 뒤 월드맵을 복원한다.
 * 인게임에서 연 설정은 항상 런 액션 영역 기준으로 시작해야 하므로 여는 쪽이 모드/상태 문구를 초기화한다.
 */
void UCombatTileMapHUDWidget::ToggleSettingsPanel()
{
	if (mCombatResultFlowActive)
	{
		return;
	}

	USettingsPanelWidget* SettingsPanelWidget = Cast<USettingsPanelWidget>(GetToggleableWorldWidget(EWorldWidgetType::InGameSettings));
	if (SettingsPanelWidget == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatTileMapHUDWidget: InGameSettings widget is not configured."));
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
		RestoreCombatControlsIfHidden();   // 지도에서 연 설정을 닫으면 전투로 복귀하므로 숨겨진 전투 컨트롤 복원.
		return;
	}

	CloseFloatingPanels(EWorldWidgetType::InGameSettings);
	SettingsPanelWidget->OnBackRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSettingsBackRequested);
	SettingsPanelWidget->OpenUI();
	SettingsPanelWidget->SetPanelMode(ESettingsPanelMode::InGame);
	SettingsPanelWidget->RefreshPanelState(false, false);
	SettingsPanelWidget->HideAbandonConfirm();
	SettingsPanelWidget->SetStatusText(FText::GetEmpty());
}

/**
 * @brief SKILL처럼 단순히 열고 닫는 플로팅 패널을 토글한다.
 *
 * @details
 * 해당 패널의 게임 로직은 각 패널이 담당하고,
 * 이 함수는 월드 위젯 등록 누락 검사와 상호배타 OpenUI/CloseUI 생명주기만 책임진다.
 */
void UCombatTileMapHUDWidget::ToggleFloatingPanel(EWorldWidgetType WorldWidgetType, const TCHAR* DebugName)
{
	if (mCombatResultFlowActive)
	{
		return;
	}

	URDUserWidget* FloatingPanel = GetToggleableWorldWidget(WorldWidgetType);
	if (FloatingPanel == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CombatTileMapHUDWidget: %s widget is not configured."), DebugName);
		return;
	}

	if (FloatingPanel->IsOpened())
	{
		FloatingPanel->CloseUI();
		RestoreCombatControlsIfHidden();
		return;
	}

	CloseFloatingPanels(WorldWidgetType);
	FloatingPanel->OpenUI();
}

/**
 * @brief 전투 결과를 승패 영상 연출로 연결한다.
 */
void UCombatTileMapHUDWidget::HandleEndCombatUI(TSharedPtr<FPresentationBarrier> Barrier)
{
	if (mCombatUIModel == nullptr)
	{
		return;
	}

	BeginCombatResultPresentation(MoveTemp(Barrier), mCombatUIModel->GetCombatResultUI().mIsWin);
}

/**
 * @brief 승리 후 다음 방 선택이 가능한 상태로 월드맵을 연다.
 *
 * @details
 * OpenUI 완료 후 presentation barrier를 해제해 전투 종료 연출과 다음 방 선택 UI의 경계가
 * 끊기지 않게 한다. OpenUI 애니메이션이 있어도 내용은 먼저 채워져 있어야 하므로 즉시
 * RefreshMap()을 부르고, 완료 콜백에서 한 번 더 갱신한다. 완료 콜백이 실제로 접근하는
 * UObject는 월드맵 위젯이므로 약한 참조 검사 대상도 월드맵 위젯으로 둔다.
 */
void UCombatTileMapHUDWidget::OpenWorldMapAfterPlayerWin()
{
	UFrontendMapWidget* WorldMapWidget = Cast<UFrontendMapWidget>(GetToggleableWorldWidget(EWorldWidgetType::WorldMap));
	if (WorldMapWidget == nullptr)
	{
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
	CloseWorldWidget(EWorldWidgetType::SkillPanel);
	WorldMapWidget->OnCloseRequested.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleWorldMapCloseRequested);
	WorldMapWidget->SetRoomSelectionEnabled(true);
	WorldMapWidget->ClearMapStatusOverride();
	WorldMapWidget->OpenUI(FOnEndUIOpenAnimation::CreateWeakLambda(WorldMapWidget, [](UUserWidget* OpenedWidget) mutable
	{
		if (UFrontendMapWidget* OpenedWorldMapWidget = Cast<UFrontendMapWidget>(OpenedWidget))
		{
			OpenedWorldMapWidget->RefreshMap();
		}
	}));
	WorldMapWidget->RefreshMap();
	SetCombatPlayControlsVisible(false);   // 승리 후 자동 지도도 MAP 버튼 지도와 동일하게 내비/룸 배너만 남긴다.
}

/**
 * @brief 승리 후 지도 잠금이 유지되는 동안 월드맵을 다시 연다.
 *
 * @details
 * 승리 후 다음 방을 고르기 전까지 월드맵은 흐름상 필수 UI다.
 * 사용자가 설정을 잠깐 열 수는 있지만, 닫힌 뒤에는 다시 다음 방 선택 지도 상태로 복원한다.
 */
void UCombatTileMapHUDWidget::RestoreVictoryWorldMap()
{
	if (!mVictoryWorldMapLocked)
	{
		return;
	}

	OpenWorldMapAfterPlayerWin();
}

/**
 * @brief 설정 패널이 열려 있으면 닫힘 완료 뒤 승리 후 월드맵을 복원한다.
 *
 * @details
 * 설정 패널 닫기 애니메이션이 있다면 그 애니메이션이 끝난 뒤 월드맵을 복원해,
 * 두 팝업이 동시에 떠 있는 순간을 줄인다.
 */
void UCombatTileMapHUDWidget::CloseSettingsPanelAndRestoreVictoryWorldMap()
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

void UCombatTileMapHUDWidget::UpdateTopBarBackdrop() const
{
	if (TopBar_Backdrop == nullptr)
	{
		return;
	}

	// 월드맵은 지도 자체가 풀스크린 배경이므로 탑바 뒤 별도 색상판을 올리지 않는다.
	// 이전처럼 여기서 배경판을 켜면 지도 상단에 파란 띠가 남는다.
	const ESlateVisibility Desired = ESlateVisibility::Collapsed;
	if (TopBar_Backdrop->GetVisibility() != Desired)
	{
		TopBar_Backdrop->SetVisibility(Desired);
	}
}

/**
 * @brief 월드맵 닫기 요청을 현재 지도 잠금 상태에 맞게 처리한다.
 *
 * @details
 * 일반 MAP 버튼으로 연 지도는 Close 요청을 그대로 닫고,
 * 승리 후 지도는 다음 방 선택을 끝내기 전까지 닫히면 안 되므로 복원으로 되돌린다.
 */
void UCombatTileMapHUDWidget::HandleWorldMapCloseRequested()
{
	if (mVictoryWorldMapLocked)
	{
		RestoreVictoryWorldMap();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::WorldMap);
	SetCombatPlayControlsVisible(true);   // 지도 닫힘(BACK 등) → 전투 컨트롤 복원.
}

/**
 * @brief 월드맵(풀스크린 지도 뷰)이 열려 있는 동안 탑바를 제외한 전투 컨트롤을 숨긴다.
 * @details 지도는 탑바 아래 레이어(z=-20)라, 안 숨기면 스킬레일/이동·턴종료가 지도 위에 뜬다.
 *          숨김/복원 대상: 런타임 컨트롤 위젯 배열 + 전투 컨트롤/상태 알약 스킨 마커(이름 접두사).
 *          내비만 유지하고, Lv/Gold/HP 알약은 지도 위에서 숨긴다.
 */
void UCombatTileMapHUDWidget::SetCombatPlayControlsVisible(bool bVisible)
{
	mCombatControlsHidden = !bVisible;

	// 지도는 HUD(z=-10)보다 아래(z=-20)에 깔린다. HUD 루트는 평소 Visible(빈 타일맵 탭까지 받으려고)이라
	// 화면 전체 입력을 먼저 삼켜서, 지도의 드래그 스크롤이 안 먹는다. 지도 열림 동안엔 루트를 SelfHitTestInvisible로
	// 바꿔 탑바 버튼(자식)만 남기고 나머지 입력을 아래 지도로 흘려보낸다. 닫으면 다시 Visible로 되돌린다.
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::SelfHitTestInvisible);

	const ESlateVisibility DisplayVis = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	const ESlateVisibility InputVis = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	auto ToggleWidgets = [](const auto& Widgets, ESlateVisibility Vis)
	{
		for (const auto& Widget : Widgets)
		{
			if (Widget != nullptr) { Widget->SetVisibility(Vis); }
		}
	};

	// 런타임 컨트롤: 스킬 레일 / 장비 칩.
	ToggleWidgets(mSkillRailPanels, DisplayVis);
	ToggleWidgets(mSkillRailTexts, DisplayVis);
	ToggleWidgets(mSkillRailIcons, DisplayVis);
	ToggleWidgets(mSkillInputButtons, InputVis);
	ToggleWidgets(mEquipmentChips, DisplayVis);
	ToggleWidgets(mEquipmentChipTexts, DisplayVis);
	ToggleWidgets(mEquipSlotButtons, InputVis);

	// mCombatStatusBarText는 항상 Collapsed(Lv/HP/Gold는 WBP HUD_M_* 라벨이 표시)이므로 여기서 손대지 않는다.
	// 복원 시 Visible로 켜면 Lv/Gold/HP 필과 같은 좌상단 위치라 빈 텍스트가 필 위에 겹친다.
	if (mMoveButton != nullptr) { mMoveButton->SetVisibility(InputVis); }
	if (EndTurnButton != nullptr) { EndTurnButton->SetVisibility(InputVis); }

	// WBP 스킨 마커(프레임 아트) — 지도 위에서는 내비/룸 배너 + 상단 상태바(Lv/Gold/HP)를 남기고 전투 컨트롤만 숨긴다.
	// (상태바 프리픽스 R_top_status_bar/HUD_M_lv_value/HUD_M_hp_value/HUD_M_gold_value는 제외 → 지도에서도 표시)
	if (DesignCanvas != nullptr)
	{
		static const TArray<FString> ControlPrefixes = {
			TEXT("R_skill_rail"), TEXT("R_btn_move"), TEXT("R_btn_end_turn"),
			TEXT("HUD_SkillRail"), TEXT("HUD_Move"), TEXT("HUD_EndTurn"),
			TEXT("HUD_M_equip"), TEXT("HUD_M_btn_move"), TEXT("HUD_M_btn_end")
		};
		const int32 ChildCount = DesignCanvas->GetChildrenCount();
		for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
		{
			UWidget* Child = DesignCanvas->GetChildAt(ChildIndex);
			if (Child == nullptr) { continue; }
			const FString ChildName = Child->GetName();
			for (const FString& Prefix : ControlPrefixes)
			{
				if (ChildName.StartsWith(Prefix))
				{
					Child->SetVisibility(DisplayVis);
					break;
				}
			}
		}
	}

}

/**
 * @brief 설정 패널의 Back 요청을 현재 지도 잠금 상태에 맞게 처리한다.
 */
void UCombatTileMapHUDWidget::HandleSettingsBackRequested()
{
	if (mVictoryWorldMapLocked)
	{
		CloseSettingsPanelAndRestoreVictoryWorldMap();
		return;
	}

	CloseWorldWidget(EWorldWidgetType::InGameSettings);
	RestoreCombatControlsIfHidden();   // 설정 Back으로 나가도 전투로 복귀하므로 숨겨진 전투 컨트롤 복원.
}

/**
 * @brief 지도/설정/스킬을 닫고 전투로 돌아올 때, 지도 뷰에서 숨겨졌던 전투 컨트롤을 복원한다.
 * @details 복원이 지도탭 토글에만 묶여 있으면 지도→설정→설정닫기처럼 지도를 거치지 않고 닫는 경로에서
 *          mCombatControlsHidden이 true로 남아 전투 UI가 되살아나지 않는다. 이 헬퍼로 모든 닫기 경로를 통일한다.
 */
void UCombatTileMapHUDWidget::RestoreCombatControlsIfHidden()
{
	if (mCombatControlsHidden)
	{
		SetCombatPlayControlsVisible(true);
	}
}
