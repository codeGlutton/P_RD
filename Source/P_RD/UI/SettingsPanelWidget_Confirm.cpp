#include "UI/SettingsPanelWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"

#define LOCTEXT_NAMESPACE "SettingsPanelWidget"

void USettingsPanelWidget::SyncRunConfirmText() const
{
	// 0823 확정: 언어 삼항 하드코딩을 걷고 로컬라이제이션(locres) 한 길로
	// 통일한다. 한국어는 ko archive 번역이 맡는다.
	const bool bSaveAndExit = mRunConfirmAction == ERunConfirmAction::SaveAndExit;
	if (RunConfirmHeaderText != nullptr)
	{
		RunConfirmHeaderText->SetText(bSaveAndExit
			? LOCTEXT("SaveAndExitHeader", "SAVE & EXIT")
			: LOCTEXT("AbandonRunHeader", "ABANDON RUN"));
	}
	if (AbandonConfirmTitleText != nullptr)
	{
		AbandonConfirmTitleText->SetText(bSaveAndExit
			? LOCTEXT("SaveAndExitConfirmTitle", "Save and exit this run?")
			: LOCTEXT("AbandonConfirmTitle", "Abandon this run?"));
	}
	if (AbandonConfirmBodyText != nullptr)
	{
		AbandonConfirmBodyText->SetText(bSaveAndExit
			? LOCTEXT("SaveAndExitConfirmBody",
				"Current progress will be saved before returning to the title.")
			: LOCTEXT("AbandonConfirmBody",
				"Current progress will be deleted and you will return to the title."));
	}
	if (ConfirmAbandonButtonText != nullptr)
	{
		ConfirmAbandonButtonText->SetText(bSaveAndExit
			? LOCTEXT("SaveAndExit", "Save and Exit")
			: LOCTEXT("Abandon", "Abandon"));
	}
	if (CancelAbandonButtonText != nullptr)
	{
		CancelAbandonButtonText->SetText(LOCTEXT("Cancel", "Cancel"));
	}
}

/**
 * @brief 런 포기 확정 패널을 표시한다.
 *
 * @details
 * "런 포기" 버튼은 위험한 액션의 1차 진입점이므로 이 함수에서는 확인 UI만 연다.
 * 실제 포기 실행은 Confirm 버튼을 눌러 OnAbandonRunConfirmed가 Broadcast된 뒤 외부 흐름에서 처리한다.
 */
void USettingsPanelWidget::ShowAbandonConfirm() const
{
	mRunConfirmAction = ERunConfirmAction::Abandon;
	SyncRunConfirmText();
	if (RunConfirmViewportLayer != nullptr)
	{
		RunConfirmViewportLayer->SetVisibility(ESlateVisibility::Visible);
	}
	if (AbandonConfirmPanel != nullptr)
	{
		AbandonConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void USettingsPanelWidget::ShowSaveAndExitConfirm() const
{
	mRunConfirmAction = ERunConfirmAction::SaveAndExit;
	SyncRunConfirmText();
	if (RunConfirmViewportLayer != nullptr)
	{
		RunConfirmViewportLayer->SetVisibility(ESlateVisibility::Visible);
	}
	if (AbandonConfirmPanel != nullptr)
	{
		AbandonConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

/**
 * @brief 런 포기 확정 패널을 숨긴다.
 *
 * @details
 * 취소하거나 패널을 초기 상태로 되돌릴 때 확인 UI만 접는다.
 * 이 함수는 확인 패널의 가시성만 바꾸며, 런 데이터나 저장 상태에는 손대지 않는다.
 */
void USettingsPanelWidget::HideAbandonConfirm() const
{
	if (RunConfirmViewportLayer != nullptr)
	{
		RunConfirmViewportLayer->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (AbandonConfirmPanel != nullptr)
	{
		AbandonConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	mRunConfirmAction = ERunConfirmAction::None;
}

/**
 * @brief 런 포기 확정 입력을 외부 이벤트로 전달한다.
 *
 * @details
 * 이 시점은 확인 패널에서 사용자가 최종 승인한 뒤다.
 * 그래도 위젯이 직접 런을 폐기하지는 않고, OnAbandonRunConfirmed를 받은 인게임 흐름이 저장/전환 순서를 책임진다.
 */
void USettingsPanelWidget::HandleConfirmAbandonButtonClicked()
{
	if (mRunConfirmAction == ERunConfirmAction::SaveAndExit)
	{
		HideAbandonConfirm();
		OnSaveAndExitRequested.Broadcast();
	}
	else if (mRunConfirmAction == ERunConfirmAction::Abandon)
	{
		HideAbandonConfirm();
		OnAbandonRunConfirmed.Broadcast();
	}
}

/**
 * @brief 런 포기 확인 취소 입력을 패널 닫기로 처리한다.
 *
 * @details
 * 취소는 순수 UI 액션이다. 외부 시스템에 알릴 필요 없이 확인 패널만 접고 기존 설정 패널 상태를 유지한다.
 */
void USettingsPanelWidget::HandleCancelAbandonButtonClicked()
{
	HideAbandonConfirm();
}

#undef LOCTEXT_NAMESPACE
