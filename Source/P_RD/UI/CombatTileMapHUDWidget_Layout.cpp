#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"
#include "UI/UIRuntimeLayout.h"

using namespace RDCombatHUD;

namespace
{
	// 주사위 판(물리 굴림) 보드/배경/캡처 위치 상수 (20260622 이식). 화면 정규화(0~1).
	constexpr float DiceRollOverlayTop = 0.045f;
	constexpr float DiceRollBoardLeft = 0.185f;
	constexpr float DiceRollBoardTop = 0.275f;
	constexpr float DiceRollBoardRight = 0.815f;
	constexpr float DiceRollBoardBottom = 0.770f;
	constexpr float DiceRollBoardInnerLeft = 0.280f;
	constexpr float DiceRollBoardInnerRight = 0.720f;
	constexpr float DiceRollBoardSingleRowCenterY = 0.522f;
	constexpr float DiceRollBoardTwoRowTopCenterY = 0.477f;
	constexpr float DiceRollBoardTwoRowGapY = 0.105f;
	constexpr float DiceRollPhysicsTop = 0.325f;
	constexpr float DiceRollPhysicsBottom = 0.695f;
	constexpr float DiceRollStatusTop = 0.710f;
	constexpr float DiceRollStatusBottom = 0.770f;
	constexpr int32 DiceRollBackdropZOrder = 300;
	constexpr int32 DiceRollBoardZOrder = 310;
	constexpr int32 DiceRollShadowZOrder = 316;
	constexpr int32 DiceRollDiceZOrder = 320;
	constexpr int32 DiceRollPhysicsZOrder = 330;
	constexpr int32 DiceRollStatusZOrder = 340;
	constexpr int32 DiceRollInputZOrder = 350;
}

void UCombatTileMapHUDWidget::ApplyRuntimeWidgetLayout() const
{
	/*
	 * 이 세 요소는 기능 API가 붙기 전의 전투 HUD 시각 검증용이다.
	 * 주사위 연출은 화면 중앙 위에 짧게 띄우고, 턴 종료는 ROLL/MOVE와 같은 우측 명령 열에 둔다.
	 */
	// 주사위 판(물리 굴림): 화면 중앙 위에 보드 배경 + 물리 캡처 + 주사위/그림자를 보드 안 레인에 배치한다.
	const int32 DiceViewportCount = mDiceRollImages.Num();
	if (mDiceRollBackdropPanel != nullptr)
	{
		RDUILayout::ApplyAnchoredSlot(mDiceRollBackdropPanel, FAnchors(0.0f, DiceRollOverlayTop, 1.0f, 1.0f), DiceRollBackdropZOrder);
	}
	if (mDiceRollBoardImage != nullptr)
	{
		RDUILayout::ApplyAnchoredSlot(mDiceRollBoardImage, FAnchors(DiceRollBoardLeft, DiceRollBoardTop, DiceRollBoardRight, DiceRollBoardBottom), DiceRollBoardZOrder);
	}
	if (mDiceRollPhysicsImage != nullptr)
	{
		RDUILayout::ApplyAnchoredSlot(mDiceRollPhysicsImage, FAnchors(0.245f, DiceRollPhysicsTop, 0.755f, DiceRollPhysicsBottom), DiceRollPhysicsZOrder);
	}
	if (DiceViewportCount > 0)
	{
		const int32 ColumnCount = DiceViewportCount <= 4 ? DiceViewportCount : 4;
		const int32 RowCount = FMath::DivideAndRoundUp(DiceViewportCount, ColumnCount);
		const float ViewportWidth = RowCount <= 1 ? 0.100f : 0.078f;
		const float ViewportHeight = ViewportWidth * 1.74f;
		const float LaneWidth = (DiceRollBoardInnerRight - DiceRollBoardInnerLeft) / StaticCast<float>(FMath::Max(1, ColumnCount));
		for (int32 DiceIndex = 0; DiceIndex < DiceViewportCount; ++DiceIndex)
		{
			const int32 ColumnIndex = DiceIndex % ColumnCount;
			const int32 RowIndex = DiceIndex / ColumnCount;
			const int32 RowItemCount = (RowIndex == RowCount - 1)
				? DiceViewportCount - RowIndex * ColumnCount
				: ColumnCount;
			const float RowLeft = 0.5f - (LaneWidth * StaticCast<float>(RowItemCount)) * 0.5f;
			const float CenterX = RowLeft + LaneWidth * (StaticCast<float>(ColumnIndex) + 0.5f);
			const float CenterY = RowCount <= 1
				? DiceRollBoardSingleRowCenterY
				: DiceRollBoardTwoRowTopCenterY + StaticCast<float>(RowIndex) * DiceRollBoardTwoRowGapY;
			const float Left = CenterX - ViewportWidth * 0.5f;
			const float Top = CenterY - ViewportHeight * 0.5f;

			if (mDiceRollShadowImages.IsValidIndex(DiceIndex))
			{
				const float ShadowWidth = ViewportWidth * 0.92f;
				const float ShadowHeight = ViewportHeight * 0.30f;
				const float ShadowLeft = CenterX - ShadowWidth * 0.5f;
				const float ShadowTop = CenterY + ViewportHeight * 0.22f;
				RDUILayout::ApplyAnchoredSlot(
					mDiceRollShadowImages[DiceIndex],
					FAnchors(ShadowLeft, ShadowTop, ShadowLeft + ShadowWidth, ShadowTop + ShadowHeight),
					DiceRollShadowZOrder + DiceIndex
				);
			}

			RDUILayout::ApplyAnchoredSlot(mDiceRollImages[DiceIndex], FAnchors(Left, Top, Left + ViewportWidth, Top + ViewportHeight), DiceRollDiceZOrder + DiceIndex);
		}
	}

	// 보유 주사위: 디자이너 스킨이면 WBP 주사위 트레이(HUD_DiceTray) 안에 세로 분배, 아니면 기존 좌하단 3열 그리드.
	const FAnchors OwnedTrayRect = GroupRect(TEXT("HUD_DiceTray"), FAnchors(0.028f, 0.790f, 0.226f, 0.998f));
	const int32 OwnedDiceCount = mOwnedDiceImages.Num();
	for (int32 DiceIndex = 0; DiceIndex < OwnedDiceCount; ++DiceIndex)
	{
		float Left = 0.0f;
		float Top = 0.0f;
		float CellWidth = 0.060f;
		float CellHeight = 0.118f;
		if (IsDesignerSkinActive())
		{
			const float GapY = 0.010f;
			const int32 Rows = FMath::Max(OwnedDiceCount, 1);
			CellHeight = (OwnedTrayRect.Maximum.Y - OwnedTrayRect.Minimum.Y - StaticCast<float>(Rows - 1) * GapY) / StaticCast<float>(Rows);
			CellWidth = OwnedTrayRect.Maximum.X - OwnedTrayRect.Minimum.X;
			Left = OwnedTrayRect.Minimum.X;
			Top = OwnedTrayRect.Minimum.Y + StaticCast<float>(DiceIndex) * (CellHeight + GapY);
		}
		else
		{
			const int32 ColumnIndex = DiceIndex % 3;
			const int32 RowIndex = DiceIndex / 3;
			Left = 0.028f + StaticCast<float>(ColumnIndex) * 0.066f;
			Top = 0.790f + StaticCast<float>(RowIndex) * 0.122f;
		}
		RDUILayout::ApplyAnchoredSlot(mOwnedDiceImages[DiceIndex], FAnchors(Left, Top, Left + CellWidth, Top + CellHeight), 22);
		if (mOwnedDiceCardWidgets.IsValidIndex(DiceIndex))
		{
			RDUILayout::ApplyAnchoredSlot(mOwnedDiceCardWidgets[DiceIndex], FAnchors(Left, Top, Left + CellWidth, Top + CellHeight), 23);
		}
		// 종류 라벨(d6/d20 등): 카드 하단에 얇게 깔고 Z를 더 높여 프리뷰 위로 보이게 한다.
		if (mOwnedDiceTypeTexts.IsValidIndex(DiceIndex))
		{
			const float LabelTop = Top + CellHeight * 0.72f;
			RDUILayout::ApplyAnchoredSlot(mOwnedDiceTypeTexts[DiceIndex], FAnchors(Left, LabelTop, Left + CellWidth, Top + CellHeight), 24);
		}
	}

	// 주사위 판 상태 문구는 보드 하단, 입력 버튼은 오버레이 전체(탭/흔들기 어디서나 받기).
	RDUILayout::ApplyAnchoredSlot(DiceRollStatusText, FAnchors(0.365f, DiceRollStatusTop, 0.635f, DiceRollStatusBottom), DiceRollStatusZOrder);
	RDUILayout::ApplyAnchoredSlot(mDiceRollInputButton, FAnchors(0.0f, DiceRollOverlayTop, 1.0f, 1.0f), DiceRollInputZOrder);
	RDUILayout::ApplyAnchoredSlot(mDiceAssignmentText, FAnchors(0.025f, 0.700f, 0.225f, 0.785f), 24);
	// 스킬 레일: 항목 위치는 WBP HUD_SkillRail(없으면 기존 상수) 영역 안에서 균등 분배한다(렌더/히트테스트 공용).
	const int32 SkillRailPanelCount = mSkillRailPanels.Num();
	for (int32 SkillIndex = 0; SkillIndex < SkillRailPanelCount; ++SkillIndex)
	{
		RDUILayout::ApplyAnchoredSlot(mSkillRailPanels[SkillIndex], GetSkillRailItemRect(SkillIndex, SkillRailPanelCount), CombatSkillRailZOrder);
	}
	const int32 SkillInputCount = mSkillInputButtons.Num();
	for (int32 SkillIndex = 0; SkillIndex < SkillInputCount; ++SkillIndex)
	{
		RDUILayout::ApplyAnchoredSlot(mSkillInputButtons[SkillIndex], GetSkillRailItemRect(SkillIndex, SkillInputCount), CombatSkillInputZOrder);
	}
	// 우측 명령 버튼: WBP HUD_EndTurn / HUD_Move 영역(없으면 기존 좌표).
	RDUILayout::ApplyAnchoredSlot(EndTurnButton, GroupRect(TEXT("HUD_EndTurn"), FAnchors(0.795f, 0.845f, 0.925f, 0.940f)), 18);
	RDUILayout::ApplyAnchoredSlot(mMoveButton, GroupRect(TEXT("HUD_Move"), FAnchors(0.795f, 0.625f, 0.925f, 0.720f)), 18);
	RDUILayout::ApplyAnchoredSlot(mCombatFeedText, FAnchors(0.350f, 0.430f, 0.650f, 0.500f), 200);
	RDUILayout::ApplyAnchoredSlot(mCombatStatusBarText, FAnchors(0.025f, 0.050f, 0.520f, 0.110f), 30);

	// 스킨 value 칸(HUD_M_*)에 Lv/HP/Gold 텍스트를 칸 위치/크기로 그린다.
	RefreshSkinValueLabels();
}

void UCombatTileMapHUDWidget::ResolveDesignerSkin()
{
	// WBP에 HUD_SkillRail 앵커 위젯이 있으면 디자이너 스킨(좌표/아트를 WBP에서 읽는) 모드로 본다.
	mDesignerSkinActive = (WidgetTree != nullptr) && (WidgetTree->FindWidget(TEXT("HUD_SkillRail")) != nullptr);
}

FAnchors UCombatTileMapHUDWidget::GroupRect(FName AnchorWidgetName, const FAnchors& Fallback) const
{
	// 디자인 기준 캔버스는 concept JSON의 designSize(1920x1080)와 같다.
	return RDUILayout::GetDesignerGroupRect(WidgetTree, AnchorWidgetName, FVector2D(1920.0f, 1080.0f), Fallback);
}

FAnchors UCombatTileMapHUDWidget::GetSkillRailGroupRect() const
{
	using namespace RDCombatHUD;
	const float FallbackBottom = CombatSkillRailTop
		+ StaticCast<float>(CombatSkillSlotCount) * CombatSkillRailHeight
		+ StaticCast<float>(CombatSkillSlotCount - 1) * CombatSkillRailGap;
	return GroupRect(TEXT("HUD_SkillRail"), FAnchors(CombatSkillRailLeft, CombatSkillRailTop, CombatSkillRailRight, FallbackBottom));
}

FAnchors UCombatTileMapHUDWidget::GetSkillRailItemRect(int32 Index, int32 Count) const
{
	using namespace RDCombatHUD;
	const FAnchors Group = GetSkillRailGroupRect();
	if (Count <= 0)
	{
		return Group;
	}
	const float GapY = CombatSkillRailGap;
	const float Height = Group.Maximum.Y - Group.Minimum.Y;
	const float ItemHeight = (Height - StaticCast<float>(Count - 1) * GapY) / StaticCast<float>(Count);
	const float ItemTop = Group.Minimum.Y + StaticCast<float>(Index) * (ItemHeight + GapY);
	return FAnchors(Group.Minimum.X, ItemTop, Group.Maximum.X, ItemTop + ItemHeight);
}
