#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatViewModel.h"

void UCombatTileMapHUDWidget::HandleCombatViewChanged(ECombatViewDomain Domain)
{
	// 상단 상태바에 영향을 주는 도메인(메타/유닛/턴/전체)만 다시 그린다.
	if (Domain == ECombatViewDomain::Meta
		|| Domain == ECombatViewDomain::Unit
		|| Domain == ECombatViewDomain::Turn
		|| Domain == ECombatViewDomain::All)
	{
		RefreshCombatStatusBar();
	}

	// 유닛 수가 바뀌면 머리 위 HP바도 다시 만든다.
	if (Domain == ECombatViewDomain::Unit || Domain == ECombatViewDomain::All)
	{
		RebuildUnitHpBars();
	}

	// 주사위(굴림/사용) 갱신 시 보유 주사위 표시를 다시 읽어 그린다(쓴 주사위 비활성 반영).
	if (Domain == ECombatViewDomain::Dice || Domain == ECombatViewDomain::All)
	{
		RefreshDiceViewsFromRunData();
		RefreshOwnedDiceCards();
	}
}

void UCombatTileMapHUDWidget::RefreshCombatStatusBar() const
{
	if (mCombatStatusBarText == nullptr)
	{
		return;
	}

	if (mCombatViewModel == nullptr)
	{
		// 뷰모델 미연결(시안 단독)에서는 상태바를 비워 둔다.
		mCombatStatusBarText->SetText(FText::GetEmpty());
		return;
	}

	// 플레이어 유닛 HP는 유닛 뷰에서, 골드/레벨은 메타에서, 라운드는 턴에서 읽는다(전부 뷰모델 경유).
	float PlayerHP = 0.f;
	float PlayerMaxHP = 0.f;
	int32 PlayerMove = 0;
	int32 PlayerMaxMove = 0;
	int32 EnemyCount = 0;
	for (const FUnitView& Unit : mCombatViewModel->GetUnitViews())
	{
		if (Unit.mIsPlayer)
		{
			PlayerHP = Unit.mHP;
			PlayerMaxHP = Unit.mMaxHP;
			PlayerMove = FMath::RoundToInt(Unit.mMovementPoint);
			PlayerMaxMove = FMath::RoundToInt(Unit.mMaxMovementPoint);
		}
		else
		{
			++EnemyCount;
		}
	}

	const FPlayerMetaView& Meta = mCombatViewModel->GetPlayerMeta();
	const FTurnView& Turn = mCombatViewModel->GetTurnView();

	const FText StatusText = FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "CombatStatusBarFormat", "HP {0}/{1}   MOVE {2}/{3}   GOLD {4}   Lv {5}   ROUND {6}   ENEMY x{7}"),
		FText::AsNumber(FMath::RoundToInt(PlayerHP)),
		FText::AsNumber(FMath::RoundToInt(PlayerMaxHP)),
		FText::AsNumber(PlayerMove),
		FText::AsNumber(PlayerMaxMove),
		FText::AsNumber(Meta.mGold),
		FText::AsNumber(Meta.mLevel),
		FText::AsNumber(Turn.mRound),
		FText::AsNumber(EnemyCount));

	mCombatStatusBarText->SetText(StatusText);
	RefreshMoveButton();
}

void UCombatTileMapHUDWidget::RefreshMoveButton() const
{
	if (mMoveButtonText == nullptr || mCombatViewModel == nullptr)
	{
		return;
	}

	int32 Move = 0;
	int32 MaxMove = 0;
	for (const FUnitView& Unit : mCombatViewModel->GetUnitViews())
	{
		if (Unit.mIsPlayer)
		{
			Move = FMath::RoundToInt(Unit.mMovementPoint);
			MaxMove = FMath::RoundToInt(Unit.mMaxMovementPoint);
			break;
		}
	}

	mMoveButtonText->SetText(FText::Format(
		NSLOCTEXT("CombatTileMapHUDWidget", "MoveCommandCount", "MOVE\n{0}/{1}"),
		FText::AsNumber(Move),
		FText::AsNumber(MaxMove)));
}

void UCombatTileMapHUDWidget::HandleMoveButtonClicked()
{
	// 이동 모드 진입 의도. 게임플레이가 이동 가능 타일을 표시하고, 타일 탭으로 이동시킨다.
	if (mCombatViewModel != nullptr)
	{
		mCombatViewModel->RequestMove();
	}
}

void UCombatTileMapHUDWidget::HandleCombatActionResolved()
{
	// 스킬/주사위 선택 강조를 푼다(액션 확정·취소 후).
	mSelectedSkillIndex = INDEX_NONE;
	mSelectedDiceIndex = INDEX_NONE;
	RefreshSkillRailWidgets();
	RefreshOwnedDiceCards();
	RefreshDiceAssignmentText();
}
