#include "UI/TopMenuBarWidget.h"

#include "Singleton/WorldSubsystem/PresentationBarrier.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "UI/FrontendMapWidget.h"

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

void UTopMenuBarWidget::UnbindCombatEvents()
{
	if (USRPGCombatSubsystem* CombatSubsystem = GetWorld() != nullptr ? GetWorld()->GetSubsystem<USRPGCombatSubsystem>() : nullptr)
	{
		CombatSubsystem->OnEndCombatUI.RemoveAll(this);
	}
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
 * OpenUI 완료 후 presentation barrier를 해제해 전투 종료 연출과 다음 방 선택 UI의 경계가 끊기지 않게 한다.
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
	CloseWorldWidget(EWorldWidgetType::DicePanel);
	CloseWorldWidget(EWorldWidgetType::SkillPanel);
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
	/*
	 * 승리 후 다음 방을 고르기 전까지 월드맵은 흐름상 필수 UI다.
	 * 사용자가 설정을 잠깐 열 수는 있지만, 닫힌 뒤에는 다시 다음 방 선택 지도 상태로 복원한다.
	 */
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
	/*
	 * 설정 패널 닫기 애니메이션이 있다면, 그 애니메이션이 끝난 뒤 월드맵을 복원한다.
	 * 동시에 두 팝업이 열리는 순간을 줄여 사용자가 보는 화면 전환이 자연스럽게 이어지게 한다.
	 */
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
