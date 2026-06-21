#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"
#include "UI/UIRuntimeLayout.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::ApplyRuntimeWidgetLayout() const
{
	/*
	 * 이 세 요소는 기능 API가 붙기 전의 전투 HUD 시각 검증용이다.
	 * 주사위 연출은 화면 중앙 위에 짧게 띄우고, 턴 종료는 ROLL/MOVE와 같은 우측 명령 열에 둔다.
	 */
	const int32 DiceViewportCount = mDiceRollImages.Num();
	if (DiceViewportCount > 0)
	{
		const float Gap = 0.012f;
		const float ViewportWidth = FMath::Min(0.135f, (0.54f - Gap * StaticCast<float>(DiceViewportCount - 1)) / StaticCast<float>(DiceViewportCount));
		const float TotalWidth = ViewportWidth * StaticCast<float>(DiceViewportCount) + Gap * StaticCast<float>(DiceViewportCount - 1);
		const float StartLeft = 0.5f - TotalWidth * 0.5f;
		for (int32 DiceIndex = 0; DiceIndex < DiceViewportCount; ++DiceIndex)
		{
			const float Left = StartLeft + StaticCast<float>(DiceIndex) * (ViewportWidth + Gap);
			// 탑바 가운데 하단 턴 순서 칩(~0.108~0.148)과 겹치지 않게 살짝 아래로.
			RDUILayout::ApplyAnchoredSlot(mDiceRollImages[DiceIndex], FAnchors(Left, 0.160f, Left + ViewportWidth, 0.390f), 80 + DiceIndex);
		}
	}

	for (int32 DiceIndex = 0; DiceIndex < mOwnedDiceImages.Num(); ++DiceIndex)
	{
		const int32 ColumnIndex = DiceIndex % 3;
		const int32 RowIndex = DiceIndex / 3;
		const float ViewportWidth = 0.060f;
		const float ViewportHeight = 0.118f;
		const float Left = 0.028f + StaticCast<float>(ColumnIndex) * 0.066f;
		const float Top = 0.790f + StaticCast<float>(RowIndex) * 0.122f;
		RDUILayout::ApplyAnchoredSlot(mOwnedDiceImages[DiceIndex], FAnchors(Left, Top, Left + ViewportWidth, Top + ViewportHeight), 22);
		if (mOwnedDiceCardWidgets.IsValidIndex(DiceIndex))
		{
			RDUILayout::ApplyAnchoredSlot(mOwnedDiceCardWidgets[DiceIndex], FAnchors(Left, Top, Left + ViewportWidth, Top + ViewportHeight), 23);
		}
		// 종류 라벨(d6/d20 등): 카드 하단에 얇게 깔고 Z를 더 높여 프리뷰 위로 보이게 한다.
		if (mOwnedDiceTypeTexts.IsValidIndex(DiceIndex))
		{
			const float LabelTop = Top + ViewportHeight * 0.72f;
			RDUILayout::ApplyAnchoredSlot(mOwnedDiceTypeTexts[DiceIndex], FAnchors(Left, LabelTop, Left + ViewportWidth, Top + ViewportHeight), 24);
		}
	}

	RDUILayout::ApplyAnchoredSlot(DiceRollStatusText, FAnchors(0.365f, 0.396f, 0.635f, 0.451f), 88);
	RDUILayout::ApplyAnchoredSlot(mDiceRollInputButton, FAnchors(0.220f, 0.150f, 0.780f, 0.460f), 140);
	RDUILayout::ApplyAnchoredSlot(mDiceAssignmentText, FAnchors(0.025f, 0.700f, 0.225f, 0.785f), 24);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailDismissButton, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), CombatSkillDetailDismissZOrder);
	if (mSkillDetailBackdropPanels.IsValidIndex(0))
	{
		RDUILayout::ApplyAnchoredSlot(
			mSkillDetailBackdropPanels[0],
			FAnchors(0.0f, 0.0f, CombatSkillRailRight + CombatSkillDetailSafeGap, 1.0f),
			CombatSkillDetailBackdropZOrder
		);
	}
	RDUILayout::ApplyAnchoredSlot(
		mSkillDetailPanel,
		FAnchors(CombatSkillRailRight + CombatSkillDetailSafeGap, 0.0f, 1.0f, 1.0f),
		CombatSkillDetailPanelZOrder
	);
	const int32 SkillRailZOrder = IsSkillDetailVisible() ? CombatSkillDetailRailZOrder : 18;
	for (int32 SkillIndex = 0; SkillIndex < mSkillRailPanels.Num(); ++SkillIndex)
	{
		const float Top = CombatSkillRailTop + StaticCast<float>(SkillIndex) * (CombatSkillRailHeight + CombatSkillRailGap);
		RDUILayout::ApplyAnchoredSlot(
			mSkillRailPanels[SkillIndex],
			FAnchors(CombatSkillRailLeft, Top, CombatSkillRailRight, Top + CombatSkillRailHeight),
			SkillRailZOrder
		);
	}
	for (int32 SkillIndex = 0; SkillIndex < mSkillInputButtons.Num(); ++SkillIndex)
	{
		const float Top = CombatSkillRailTop + StaticCast<float>(SkillIndex) * (CombatSkillRailHeight + CombatSkillRailGap);
		RDUILayout::ApplyAnchoredSlot(
			mSkillInputButtons[SkillIndex],
			FAnchors(CombatSkillRailLeft, Top, CombatSkillRailRight, Top + CombatSkillRailHeight),
			CombatSkillInputZOrder
		);
	}
	RDUILayout::ApplyAnchoredSlot(EndTurnButton, FAnchors(0.795f, 0.845f, 0.925f, 0.940f), 18);
	RDUILayout::ApplyAnchoredSlot(mMoveButton, FAnchors(0.795f, 0.625f, 0.925f, 0.720f), 18);
	RDUILayout::ApplyAnchoredSlot(mCombatFeedText, FAnchors(0.350f, 0.430f, 0.650f, 0.500f), 200);
	RDUILayout::ApplyAnchoredSlot(mCombatStatusBarText, FAnchors(0.025f, 0.050f, 0.520f, 0.110f), 30);
}
