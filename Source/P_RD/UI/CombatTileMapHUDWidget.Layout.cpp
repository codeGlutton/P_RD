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
			RDUILayout::ApplyAnchoredSlot(mDiceRollImages[DiceIndex], FAnchors(Left, 0.105f, Left + ViewportWidth, 0.335f), 80 + DiceIndex);
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
	}

	RDUILayout::ApplyAnchoredSlot(DiceRollStatusText, FAnchors(0.365f, 0.342f, 0.635f, 0.397f), 88);
	RDUILayout::ApplyAnchoredSlot(mDiceRollInputButton, FAnchors(0.220f, 0.090f, 0.780f, 0.415f), 140);
	RDUILayout::ApplyAnchoredSlot(mDiceAssignmentText, FAnchors(0.025f, 0.700f, 0.225f, 0.785f), 24);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailDismissButton, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), 159);
	RDUILayout::ApplyAnchoredSlot(mSkillDetailPanel, FAnchors(0.225f, 0.505f, 0.470f, 0.705f), 160);
	for (int32 SkillIndex = 0; SkillIndex < mSkillRailPanels.Num(); ++SkillIndex)
	{
		const float Top = CombatSkillRailTop + StaticCast<float>(SkillIndex) * (CombatSkillRailHeight + CombatSkillRailGap);
		RDUILayout::ApplyAnchoredSlot(
			mSkillRailPanels[SkillIndex],
			FAnchors(CombatSkillRailLeft, Top, CombatSkillRailRight, Top + CombatSkillRailHeight),
			18
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
}
