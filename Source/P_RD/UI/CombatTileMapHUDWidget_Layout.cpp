#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "P_RD.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"
#include "UI/UIRuntimeLayout.h"

using namespace RDCombatHUD;

namespace
{
	// 주사위 판(물리 굴림) 보드/배경/캡처 위치 상수 (20260622 이식). 화면 정규화(0~1).
	constexpr float DiceRollOverlayTop = 0.045f;
	// 판을 상/하로 꽉 채운다(세로 13~96%, 탑바 아래부터 바닥 근처까지). 눈으로 튜닝하는 값.
	constexpr float DiceRollBoardLeft = 0.04f;
	constexpr float DiceRollBoardTop = 0.13f;
	constexpr float DiceRollBoardRight = 0.96f;
	constexpr float DiceRollBoardBottom = 0.96f;
	constexpr float DiceRollPhysicsTop = 0.19f;
	constexpr float DiceRollPhysicsBottom = 0.89f;
	constexpr float DiceRollStatusTop = 0.710f;
	constexpr float DiceRollStatusBottom = 0.770f;
	constexpr int32 DiceRollBackdropZOrder = 300;
	constexpr int32 DiceRollBoardZOrder = 310;
	constexpr int32 DiceRollPhysicsZOrder = 330;
	constexpr int32 DiceRollStatusZOrder = 340;
	// 입장 굴림 오버레이는 모달이다 — 입력 레이어가 스킬 입력(CombatSkillInputZOrder=1000)보다 위에 있어야
	// 오버레이 위 아무 곳이나 탭해 닫을 수 있고, 닫기 전엔 스킬/주사위 클릭이 새어 들어가지 않는다.
	constexpr int32 DiceRollInputZOrder = RDCombatHUD::CombatSkillInputZOrder + 100;

	FAnchorData MakeCenteredSquareSlot(const FAnchorData& CellSlot)
	{
		FAnchorData SquareSlot = CellSlot;
		const float SquareSize = FMath::Min(CellSlot.Offsets.Right, CellSlot.Offsets.Bottom);
		SquareSlot.Offsets.Left += (CellSlot.Offsets.Right - SquareSize) * 0.5f;
		SquareSlot.Offsets.Top += (CellSlot.Offsets.Bottom - SquareSize) * 0.5f;
		SquareSlot.Offsets.Right = SquareSize;
		SquareSlot.Offsets.Bottom = SquareSize;
		return SquareSlot;
	}

}

void UCombatTileMapHUDWidget::ApplyRuntimeWidgetLayout() const
{
	/*
	 * 이 요소들은 기능 API가 붙기 전의 전투 HUD 시각 검증용이다. 턴 종료는 ROLL/MOVE와 같은 우측 명령 열에 둔다.
	 */
	// 스킨(엣지 피닝): 마커의 슬롯 데이터(핀 앵커 + 디자인px 오프셋)를 그대로 복사한다 — 구 WBP((0,0) 점앵커)와
	// 신 WBP(엣지 핀 점앵커) 모두에서 동일하게 동작한다(C++ 선배포 안전). 레거시(스킨 없음)는 기존 정규화 경로.
	const bool bSkin = IsDesignerSkinActive();
	const FVector2D DesignSize(1920.0f, 1080.0f);

	// 주사위 판(물리 굴림) 오버레이: WBP-네이티브 체인(DiceRollDesignCanvas)이 있으면 WBP ScaleBox가
	// 종횡비/위치를 소유하므로 슬롯을 건드리지 않는다. 단 WBP에 구운 입력막 z(350)는 스킬 입력
	// (CombatSkillInputZOrder=1000)을 못 가리므로 모달 보장을 위해 ZOrder만 승격한다.
	const bool bDiceOverlayWbpNative = (WidgetTree != nullptr) && (WidgetTree->FindWidget(TEXT("DiceRollDesignCanvas")) != nullptr);
	if (bDiceOverlayWbpNative)
	{
		if (UCanvasPanelSlot* DiceRollInputSlot = RDUILayout::GetCanvasSlot(mDiceRollInputButton))
		{
			DiceRollInputSlot->SetZOrder(DiceRollInputZOrder);
		}
	}
	else
	{
		// 레거시(런타임 생성) 오버레이: 화면 중앙 위에 보드 배경 + 물리 캡처를 정규화 상수로 배치한다.
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
			RDUILayout::ApplyAnchoredSlot(mDiceRollPhysicsImage, FAnchors(0.10f, DiceRollPhysicsTop, 0.90f, DiceRollPhysicsBottom), DiceRollPhysicsZOrder);
		}
		// 상태 문구는 보드 하단, 입력 버튼은 오버레이 전체(탭/흔들기 어디서나 받기).
		RDUILayout::ApplyAnchoredSlot(DiceRollStatusText, FAnchors(0.365f, DiceRollStatusTop, 0.635f, DiceRollStatusBottom), DiceRollStatusZOrder);
		RDUILayout::ApplyAnchoredSlot(mDiceRollInputButton, FAnchors(0.0f, DiceRollOverlayTop, 1.0f, 1.0f), DiceRollInputZOrder);
	}

	// 보유 주사위: 스킨이면 HUD_DiceTray 마커 슬롯을 상속해 세로 분배, 아니면 기존 좌하단 3열 그리드.
	const int32 OwnedDiceCount = mOwnedDiceImages.Num();
	if (bSkin)
	{
		const FAnchorData TraySlot = RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_DiceTray"), FAnchors(0.028f, 0.790f, 0.226f, 0.998f), DesignSize);
		for (int32 DiceIndex = 0; DiceIndex < OwnedDiceCount; ++DiceIndex)
		{
			const FAnchorData CellSlot = RDUILayout::MakeVerticalSubSlot(TraySlot, DiceIndex, FMath::Max(OwnedDiceCount, 1), 10.8f);   // 0.010*1080
			const FAnchorData DiceImageSlot = MakeCenteredSquareSlot(CellSlot);
			RDUILayout::ApplyDesignerSlotData(mOwnedDiceImages[DiceIndex], DiceImageSlot, 22);
			if (mOwnedDiceCardWidgets.IsValidIndex(DiceIndex))
			{
				RDUILayout::ApplyDesignerSlotData(mOwnedDiceCardWidgets[DiceIndex], CellSlot, 23);
			}
			if (mOwnedDiceTypeTexts.IsValidIndex(DiceIndex))
			{
				if (mOwnedDiceTypeTexts[DiceIndex] != nullptr)
				{
					mOwnedDiceTypeTexts[DiceIndex]->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
	else
	{
		for (int32 DiceIndex = 0; DiceIndex < OwnedDiceCount; ++DiceIndex)
		{
			const int32 ColumnIndex = DiceIndex % 3;
			const int32 RowIndex = DiceIndex / 3;
			const float Left = 0.028f + StaticCast<float>(ColumnIndex) * 0.066f;
			const float Top = 0.790f + StaticCast<float>(RowIndex) * 0.122f;
			const float CellWidth = 0.060f;
			const float CellHeight = 0.118f;
			const float DiceSize = FMath::Min(CellWidth, CellHeight);
			const float DiceLeft = Left + (CellWidth - DiceSize) * 0.5f;
			const float DiceTop = Top + (CellHeight - DiceSize) * 0.5f;
			const FAnchors DiceImageAnchors(DiceLeft, DiceTop, DiceLeft + DiceSize, DiceTop + DiceSize);
			RDUILayout::ApplyAnchoredSlot(mOwnedDiceImages[DiceIndex], DiceImageAnchors, 22);
			if (mOwnedDiceCardWidgets.IsValidIndex(DiceIndex))
			{
				RDUILayout::ApplyAnchoredSlot(mOwnedDiceCardWidgets[DiceIndex], FAnchors(Left, Top, Left + CellWidth, Top + CellHeight), 23);
			}
			if (mOwnedDiceTypeTexts.IsValidIndex(DiceIndex))
			{
				if (mOwnedDiceTypeTexts[DiceIndex] != nullptr)
				{
					mOwnedDiceTypeTexts[DiceIndex]->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}

	// 어사인먼트 힌트: 스킨이면 트레이 열 오른쪽(디자인px 300~720, 하단) - 레일/트레이 열과 겹치지 않는 빈 보드 영역.
	// 레거시는 기존 좌하단 정규화 좌표 유지.
	if (bSkin)
	{
		RDUILayout::ApplyDesignerSlotData(mDiceAssignmentText, RDUILayout::NormalizedToDesignPointSlot(FAnchors(0.156f, 0.759f, 0.375f, 0.852f), DesignSize), 24);
	}
	else
	{
		RDUILayout::ApplyAnchoredSlot(mDiceAssignmentText, FAnchors(0.025f, 0.700f, 0.225f, 0.785f), 24);
	}
	// concept_09 상세는 HP바(z210)·스킬레일(z170)보다 위에 떠야 뒤 요소가 안 뚫려 보인다. → z245+로 올리고,
	// 그 아래 풀뷰포트 회색 딤(주사위 배경과 동일)을 깔아 뒤 HUD를 덮는다. WBP는 전체 화면(내부 레이아웃/아트는 WBP 소유).
	if (mSkillDetailBackdropPanels.IsValidIndex(0) && mSkillDetailBackdropPanels[0] != nullptr)
	{
		RDUILayout::ApplyAnchoredSlot(mSkillDetailBackdropPanels[0], FAnchors(0.0f, 0.0f, 1.0f, 1.0f), 245);   // 풀뷰포트 회색 딤
	}
	RDUILayout::ApplyAnchoredSlot(mDetailOverlay, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), 246);            // 상세 WBP(패널/아트)
	RDUILayout::ApplyAnchoredSlot(mSkillDetailDismissButton, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), 248); // 아무 데나 탭 닫기

	// 장비 슬롯 롱프레스 감지 버튼을 각 장비 마커(HUD_M_equip_i) 위에 얹는다(스킬 상세와 동일하게 마커 슬롯 상속).
	static const TCHAR* const EquipMarkerNames[] = { TEXT("HUD_M_equip_0"), TEXT("HUD_M_equip_1"), TEXT("HUD_M_equip_2") };
	for (int32 SlotIndex = 0; SlotIndex < mEquipSlotButtons.Num() && SlotIndex < UE_ARRAY_COUNT(EquipMarkerNames); ++SlotIndex)
	{
		if (mEquipSlotButtons[SlotIndex] != nullptr)
		{
			RDUILayout::ApplyDesignerSlotData(
				mEquipSlotButtons[SlotIndex],
				RDUILayout::GetDesignerSlotDataOr(WidgetTree, EquipMarkerNames[SlotIndex], FAnchors(0.02f, 0.10f, 0.06f, 0.14f), DesignSize),
				40);
		}
	}
	// 스킬 레일: 스킨이면 HUD_SkillRail 마커 슬롯 상속 세로 분배(렌더/입력 동일), 레거시는 기존 정규화 분배.
	// 상세가 열려 있으면 레일 Z를 승격(현행 동작 유지) — 아이콘 +1, 라벨 +2로 3상태 렌더링 위계도 그대로다.
	const int32 SkillRailZOrder = IsSkillDetailVisible() ? CombatSkillDetailRailZOrder : 18;
	const int32 SkillRailPanelCount = mSkillRailPanels.Num();
	const int32 SkillInputCount = mSkillInputButtons.Num();
	if (bSkin)
	{
		const float RailFallbackBottom = CombatSkillRailTop
			+ StaticCast<float>(CombatSkillSlotCount) * CombatSkillRailHeight
			+ StaticCast<float>(CombatSkillSlotCount - 1) * CombatSkillRailGap;
		const FAnchorData RailSlot = RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_SkillRail"),
			FAnchors(CombatSkillRailLeft, CombatSkillRailTop, CombatSkillRailRight, RailFallbackBottom), DesignSize);
		// 스킨 프레임 아트의 실제 슬롯을 기준으로 아이콘과 입력 버튼을 맞춘다.
		// 프레임을 못 찾으면 기존처럼 레일 영역을 등분한다.
		const TArray<FAnchorData> RailFrameSlots = RDUILayout::CollectPointSlotsByPrefix(WidgetTree, TEXT("R_skill_rail_slot"));
		const float RailGapPx = 20.0f;
		for (int32 SkillIndex = 0; SkillIndex < SkillRailPanelCount; ++SkillIndex)
		{
			const FAnchorData CellSlot = RailFrameSlots.IsValidIndex(SkillIndex)
				? RailFrameSlots[SkillIndex]
				: RDUILayout::MakeVerticalSubSlot(RailSlot, SkillIndex, SkillRailPanelCount, RailGapPx);
			RDUILayout::ApplyDesignerSlotData(mSkillRailPanels[SkillIndex], CellSlot, SkillRailZOrder);

			// 보유 스킬 아이콘: 라벨 없이 아이콘만 - 프레임 안 중앙(가로 여백 15%, 세로 여백 8%).
			const float CellWidthPx = CellSlot.Offsets.Right;    // 점앵커 슬롯: Right/Bottom = 크기(px).
			const float CellHeightPx = CellSlot.Offsets.Bottom;
			if (mSkillRailIcons.IsValidIndex(SkillIndex))
			{
				FAnchorData IconSlot = CellSlot;
				IconSlot.Offsets.Left += CellWidthPx * 0.15f;
				IconSlot.Offsets.Top += CellHeightPx * 0.08f;
				IconSlot.Offsets.Right = CellWidthPx * 0.70f;
				IconSlot.Offsets.Bottom = CellHeightPx * 0.84f;
				RDUILayout::ApplyDesignerSlotData(mSkillRailIcons[SkillIndex], IconSlot, SkillRailZOrder + 1);
			}
		}
		for (int32 SkillIndex = 0; SkillIndex < SkillInputCount; ++SkillIndex)
		{
			// 보이는 칸과 입력 영역이 어긋나지 않게 같은 슬롯을 쓴다.
			const FAnchorData ButtonSlot = RailFrameSlots.IsValidIndex(SkillIndex)
				? RailFrameSlots[SkillIndex]
				: RDUILayout::MakeVerticalSubSlot(RailSlot, SkillIndex, SkillInputCount, RailGapPx);
			RDUILayout::ApplyDesignerSlotData(mSkillInputButtons[SkillIndex], ButtonSlot, CombatSkillInputZOrder);
		}
	}
	else
	{
		for (int32 SkillIndex = 0; SkillIndex < SkillRailPanelCount; ++SkillIndex)
		{
			const FAnchors ItemRect = GetSkillRailItemRect(SkillIndex, SkillRailPanelCount);
			RDUILayout::ApplyAnchoredSlot(mSkillRailPanels[SkillIndex], ItemRect, SkillRailZOrder);

			// 보유 스킬 아이콘: 라벨 없이 아이콘만 - 슬롯 안 중앙(가로 여백 15%, 세로 여백 8%).
			const float ItemW = ItemRect.Maximum.X - ItemRect.Minimum.X;
			const float ItemH = ItemRect.Maximum.Y - ItemRect.Minimum.Y;
			if (mSkillRailIcons.IsValidIndex(SkillIndex))
			{
				const FAnchors IconRect(
					ItemRect.Minimum.X + ItemW * 0.15f, ItemRect.Minimum.Y + ItemH * 0.08f,
					ItemRect.Maximum.X - ItemW * 0.15f, ItemRect.Maximum.Y - ItemH * 0.08f);
				RDUILayout::ApplyAnchoredSlot(mSkillRailIcons[SkillIndex], IconRect, SkillRailZOrder + 1);
			}
		}
		for (int32 SkillIndex = 0; SkillIndex < SkillInputCount; ++SkillIndex)
		{
			RDUILayout::ApplyAnchoredSlot(mSkillInputButtons[SkillIndex], GetSkillRailItemRect(SkillIndex, SkillInputCount), CombatSkillInputZOrder);
		}
	}

	if (bSkin)
	{
		// 우측 명령/상단 Nav/피드·상태 텍스트: 마커 슬롯 복사(핀 상속). 마커 없으면 디자인px 좌상단 fallback.
		RDUILayout::ApplyDesignerSlotData(EndTurnButton, RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_EndTurn"), FAnchors(0.795f, 0.845f, 0.925f, 0.940f), DesignSize), 18);
		RDUILayout::ApplyDesignerSlotData(mMoveButton, RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_Move"), FAnchors(0.795f, 0.625f, 0.925f, 0.720f), DesignSize), 18);
		RDUILayout::ApplyDesignerSlotData(mNavMapButton, RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_Map"), FAnchors(0.7448f, 0.0185f, 0.7969f, 0.0963f), DesignSize), 19);
		RDUILayout::ApplyDesignerSlotData(mNavDiceButton, RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_Dice"), FAnchors(0.8031f, 0.0185f, 0.8552f, 0.0963f), DesignSize), 19);
		RDUILayout::ApplyDesignerSlotData(mNavSkillButton, RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_Skill"), FAnchors(0.8615f, 0.0185f, 0.9135f, 0.0963f), DesignSize), 19);
		RDUILayout::ApplyDesignerSlotData(mNavSettingsButton, RDUILayout::GetDesignerSlotDataOr(WidgetTree, TEXT("HUD_Settings"), FAnchors(0.9198f, 0.0185f, 0.9719f, 0.0963f), DesignSize), 19);
		// 피드: 화면 중앙 핀(디자인px). 상태바 텍스트: 좌상단 핀(스킨에선 Collapsed지만 좌표계 일관 유지).
		FAnchorData FeedSlot;
		FeedSlot.Anchors = FAnchors(0.5f, 0.5f, 0.5f, 0.5f);
		FeedSlot.Alignment = FVector2D::ZeroVector;
		FeedSlot.Offsets = FMargin(-288.0f, -75.6f, 576.0f, 75.6f);   // 0.35~0.65 x 0.43~0.50 의 중앙 상대 px
		RDUILayout::ApplyDesignerSlotData(mCombatFeedText, FeedSlot, 200);
		RDUILayout::ApplyDesignerSlotData(mCombatStatusBarText, RDUILayout::NormalizedToDesignPointSlot(FAnchors(0.025f, 0.050f, 0.520f, 0.110f), DesignSize), 30);
	}
	else
	{
		// 우측 명령 버튼과 탑바 내비 투명 버튼: 상수 폴백 좌표(레거시 시안).
		RDUILayout::ApplyAnchoredSlot(EndTurnButton, FAnchors(0.795f, 0.845f, 0.925f, 0.940f), 18);
		RDUILayout::ApplyAnchoredSlot(mMoveButton, FAnchors(0.795f, 0.625f, 0.925f, 0.720f), 18);
		RDUILayout::ApplyAnchoredSlot(mNavMapButton, FAnchors(0.7448f, 0.0185f, 0.7969f, 0.0963f), 19);
		RDUILayout::ApplyAnchoredSlot(mNavDiceButton, FAnchors(0.8031f, 0.0185f, 0.8552f, 0.0963f), 19);
		RDUILayout::ApplyAnchoredSlot(mNavSkillButton, FAnchors(0.8615f, 0.0185f, 0.9135f, 0.0963f), 19);
		RDUILayout::ApplyAnchoredSlot(mNavSettingsButton, FAnchors(0.9198f, 0.0185f, 0.9719f, 0.0963f), 19);
		RDUILayout::ApplyAnchoredSlot(mCombatFeedText, FAnchors(0.350f, 0.430f, 0.650f, 0.500f), 200);
		RDUILayout::ApplyAnchoredSlot(mCombatStatusBarText, FAnchors(0.025f, 0.050f, 0.520f, 0.110f), 30);
	}

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

void UCombatTileMapHUDWidget::LogCombatLayoutMetrics(const FVector2D& ViewportSize) const
{
	const float Aspect = ViewportSize.Y > 0.0f ? ViewportSize.X / ViewportSize.Y : 0.0f;

	// 스킨 모드에서 실제 UI가 얹히는 캔버스(디자인 1920x1080)와 루트(풀 뷰포트) 크기를 함께 남긴다.
	FVector2D DesignLocal = FVector2D::ZeroVector;
	if (DesignCanvas != nullptr)
	{
		DesignLocal = DesignCanvas->GetCachedGeometry().GetLocalSize();
	}
	FVector2D RootLocal = FVector2D::ZeroVector;
	if (RootCanvas != nullptr)
	{
		RootLocal = RootCanvas->GetCachedGeometry().GetLocalSize();
	}

	// 각 HUD_* 그룹이 WBP 앵커에서 왔는지(wbp) 코드 fallback인지(fb) + 해석된 정규화 rect를 표기한다.
	auto DescribeGroup = [this](const TCHAR* AnchorName, const FAnchors& Fallback) -> FString
	{
		const FName AnchorId(AnchorName);
		const bool bFromWbp = (WidgetTree != nullptr) && (WidgetTree->FindWidget(AnchorId) != nullptr);
		const FAnchors Rect = GroupRect(AnchorId, Fallback);
		return FString::Printf(TEXT("%s{%s %.3f,%.3f..%.3f,%.3f}"),
			AnchorName, bFromWbp ? TEXT("wbp") : TEXT("fb"),
			Rect.Minimum.X, Rect.Minimum.Y, Rect.Maximum.X, Rect.Maximum.Y);
	};

	const bool bSkillRailWbp = (WidgetTree != nullptr) && (WidgetTree->FindWidget(TEXT("HUD_SkillRail")) != nullptr);
	const FAnchors SkillRail = GetSkillRailGroupRect();

	UE_LOG(LogRD, Display,
		TEXT("CombatHUD LayoutMetrics: viewport=%.1f,%.1f aspect=%.3f skin=%s designNorm=1920.0,1080.0 designCanvas=%.1f,%.1f rootCanvas=%.1f,%.1f skillRail={%s %.3f,%.3f..%.3f,%.3f}"),
		ViewportSize.X, ViewportSize.Y, Aspect,
		IsDesignerSkinActive() ? TEXT("on") : TEXT("off"),
		DesignLocal.X, DesignLocal.Y, RootLocal.X, RootLocal.Y,
		bSkillRailWbp ? TEXT("wbp") : TEXT("fb"),
		SkillRail.Minimum.X, SkillRail.Minimum.Y, SkillRail.Maximum.X, SkillRail.Maximum.Y);

	UE_LOG(LogRD, Display,
		TEXT("CombatHUD LayoutMetrics Groups: %s %s %s %s %s %s %s %s"),
		*DescribeGroup(TEXT("HUD_DiceTray"), FAnchors(0.028f, 0.790f, 0.226f, 0.998f)),
		*DescribeGroup(TEXT("HUD_Move"), FAnchors(0.795f, 0.625f, 0.925f, 0.720f)),
		*DescribeGroup(TEXT("HUD_EndTurn"), FAnchors(0.795f, 0.845f, 0.925f, 0.940f)),
		*DescribeGroup(TEXT("HUD_Map"), FAnchors(0.7448f, 0.0185f, 0.7969f, 0.0963f)),
		*DescribeGroup(TEXT("HUD_Dice"), FAnchors(0.8031f, 0.0185f, 0.8552f, 0.0963f)),
		*DescribeGroup(TEXT("HUD_Skill"), FAnchors(0.8615f, 0.0185f, 0.9135f, 0.0963f)),
		*DescribeGroup(TEXT("HUD_Settings"), FAnchors(0.9198f, 0.0185f, 0.9719f, 0.0963f)),
		*DescribeGroup(TEXT("HUD_SkillDetail"), FAnchors(0.315f, 0.215f, 0.685f, 0.585f)));
}

void UCombatTileMapHUDWidget::ApplyAspectVariantSlots(const FVector2D& ViewportSize)
{
	if (IsDesignerSkinActive() == false || ViewportSize.X <= 1.0f || ViewportSize.Y <= 1.0f)
	{
		return;
	}
	// 컷은 concept_02 responsive.fold_narrow(aspectMax=1.68)와 동기 — 변경 시 JSON과 함께 수정할 것.
	constexpr float FoldAspectCut = 1.68f;
	const bool bFold = (ViewportSize.X / ViewportSize.Y) < FoldAspectCut;
	if (bFold == mFoldVariantActive)
	{
		return;
	}

	// 클러스터: 첫 위젯이 y델타 기준 아트(요소 rect와 동일 좌표) — 폴드 마커.Top - 베이스.Top이 곧 이동량(디자인px).
	static const FName RoomClusterWidgets[] = { FName(TEXT("R_room_name_room_name_7")), FName(TEXT("HUD_M_room_name_text")) };
	static const FName TurnClusterWidgets[] = { FName(TEXT("R_turn_order_turn_order_16")), FName(TEXT("HUD_M_turn_order_tokens")) };

	auto ApplyCluster = [this, bFold](const TCHAR* FoldMarkerName, const FName* ClusterWidgets, int32 WidgetCount) -> float
	{
		FAnchorData FoldSlot;
		if (RDUILayout::GetDesignerSlotData(WidgetTree, FoldMarkerName, FoldSlot) == false)
		{
			return 0.0f;   // 폴드 마커가 없는 WBP(구 스킨) — 변형 없이 기준 유지.
		}
		float DeltaY = 0.0f;
		for (int32 WidgetIndex = 0; WidgetIndex < WidgetCount; ++WidgetIndex)
		{
			UCanvasPanelSlot* TargetSlot = RDUILayout::GetCanvasSlot(WidgetTree->FindWidget(ClusterWidgets[WidgetIndex]));
			if (TargetSlot == nullptr)
			{
				continue;
			}
			FAnchorData* BaseSlot = mAspectVariantBaseSlots.Find(ClusterWidgets[WidgetIndex]);
			if (BaseSlot == nullptr)
			{
				BaseSlot = &mAspectVariantBaseSlots.Add(ClusterWidgets[WidgetIndex], TargetSlot->GetLayout());
			}
			if (WidgetIndex == 0)
			{
				DeltaY = FoldSlot.Offsets.Top - BaseSlot->Offsets.Top;
			}
			FAnchorData NewSlot = *BaseSlot;
			if (bFold)
			{
				NewSlot.Offsets.Top += DeltaY;
			}
			TargetSlot->SetLayout(NewSlot);
		}
		return DeltaY;
	};

	ApplyCluster(TEXT("HUD_RoomNameFold"), RoomClusterWidgets, UE_ARRAY_COUNT(RoomClusterWidgets));
	const float TurnDeltaY = ApplyCluster(TEXT("HUD_TurnOrderFold"), TurnClusterWidgets, UE_ARRAY_COUNT(TurnClusterWidgets));

	mFoldVariantActive = bFold;
	mFoldTurnOrderDeltaY = bFold ? TurnDeltaY : 0.0f;
	RebuildTurnOrderBar();   // 런타임 칩 줄도 새 y로 다시 배치(모델 없으면 내부에서 no-op).
}
