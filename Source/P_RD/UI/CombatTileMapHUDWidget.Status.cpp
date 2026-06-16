#include "UI/CombatTileMapHUDWidget.h"

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
}
