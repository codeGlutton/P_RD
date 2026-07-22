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
	constexpr int32 TurnChangeBackdropZOrder = RDCombatHUD::CombatSkillInputZOrder + 180;
	constexpr int32 TurnChangeInputZOrder = RDCombatHUD::CombatSkillInputZOrder + 190;
	constexpr int32 TurnChangeVideoZOrder = RDCombatHUD::CombatSkillInputZOrder + 200;
	constexpr int32 TurnChangeTextZOrder = RDCombatHUD::CombatSkillInputZOrder + 210;

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

	RDUILayout::ApplyAnchoredSlot(mTurnChangeBackdropPanel, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), TurnChangeBackdropZOrder);
	RDUILayout::ApplyAnchoredSlot(mTurnChangeInputBlocker, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), TurnChangeInputZOrder);
	RDUILayout::ApplyAnchoredSlot(mTurnChangeVideoImage, FAnchors(0.140f, 0.155f, 0.860f, 0.845f), TurnChangeVideoZOrder);
	RDUILayout::ApplyAnchoredSlot(mTurnChangeTurnTextPanel, FAnchors(0.435f, 0.390f, 0.565f, 0.610f), TurnChangeTextZOrder);

	// concept_09 상세는 HP바(z210)·스킬레일(z170)보다 위에 떠야 뒤 요소가 안 뚫려 보인다. → z245+로 올리고,
	// 그 아래 풀뷰포트 회색 딤을 깔아 뒤 HUD를 덮는다. WBP는 전체 화면(내부 레이아웃/아트는 WBP 소유).
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
		TEXT("CombatHUD LayoutMetrics Groups: %s %s %s %s %s %s"),
		*DescribeGroup(TEXT("HUD_Move"), FAnchors(0.795f, 0.625f, 0.925f, 0.720f)),
		*DescribeGroup(TEXT("HUD_EndTurn"), FAnchors(0.795f, 0.845f, 0.925f, 0.940f)),
		*DescribeGroup(TEXT("HUD_Map"), FAnchors(0.7448f, 0.0185f, 0.7969f, 0.0963f)),
		*DescribeGroup(TEXT("HUD_Skill"), FAnchors(0.8615f, 0.0185f, 0.9135f, 0.0963f)),
		*DescribeGroup(TEXT("HUD_Settings"), FAnchors(0.9198f, 0.0185f, 0.9719f, 0.0963f)),
		*DescribeGroup(TEXT("HUD_SkillDetail"), FAnchors(0.315f, 0.215f, 0.685f, 0.585f)));
}
