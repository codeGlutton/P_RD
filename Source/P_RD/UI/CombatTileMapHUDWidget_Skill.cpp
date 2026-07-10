#include "UI/CombatTileMapHUDWidget.h"

#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"

#define LOCTEXT_NAMESPACE "CombatTileMapHUDWidget_Skill"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::HandleEndTurnButtonClicked()
{
	// 뷰모델 연결 시 턴 종료는 의도로 보낸다. 미연결 시 기존처럼 로그만.
	if (mCombatUIModel != nullptr)
	{
		mCombatUIModel->RequestEndTurn();
		return;
	}

	UE_LOG(LogRD, Log, TEXT("END TURN button clicked. Combat turn API is not connected yet."));
}

void UCombatTileMapHUDWidget::RefreshDiceAssignmentText() const
{
	// 주사위 배치 안내 문구("Tap a rolled die" 등) 영역은 표시하지 않기로 함(20260710 요청).
	// 배치 상태는 주사위 색(진행중=노랑/완료=초록/미배치=회색)이 이미 전달한다.
	if (mDiceAssignmentText != nullptr)
	{
		mDiceAssignmentText->SetText(FText::GetEmpty());
		mDiceAssignmentText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
