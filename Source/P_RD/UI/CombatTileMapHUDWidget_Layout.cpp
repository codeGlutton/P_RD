#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
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
	constexpr int32 TurnChangeBackdropZOrder = RDCombatHUD::CombatSkillInputZOrder + 180;
	constexpr int32 TurnChangeInputZOrder = RDCombatHUD::CombatSkillInputZOrder + 190;
	constexpr int32 TurnChangeVideoZOrder = RDCombatHUD::CombatSkillInputZOrder + 200;
	constexpr int32 TurnChangeTextZOrder = RDCombatHUD::CombatSkillInputZOrder + 210;

	FAnchorData MakePointSlot(float Left, float Top, float Width, float Height)
	{
		FAnchorData Slot;
		Slot.Anchors = FAnchors(0.0f, 0.0f);
		Slot.Alignment = FVector2D::ZeroVector;
		Slot.Offsets = FMargin(Left, Top, Width, Height);
		return Slot;
	}

	FAnchorData MakePinnedSlot(const FAnchors& Anchors, float Left, float Top, float Width, float Height)
	{
		FAnchorData Slot;
		Slot.Anchors = Anchors;
		Slot.Alignment = FVector2D::ZeroVector;
		Slot.Offsets = FMargin(Left, Top, Width, Height);
		return Slot;
	}

}

void UCombatTileMapHUDWidget::ApplyRuntimeWidgetLayout() const
{
	/*
	 * 이 요소들은 기능 API가 붙기 전의 전투 HUD 시각 검증용이다.
	 */
	// 스킨(엣지 피닝): 마커의 슬롯 데이터(핀 앵커 + 디자인px 오프셋)를 그대로 복사한다 — 구 WBP((0,0) 점앵커)와
	// 신 WBP(엣지 핀 점앵커) 모두에서 동일하게 동작한다(C++ 선배포 안전). 레거시(스킨 없음)는 기존 정규화 경로.
	const bool bSkin = IsDesignerSkinActive();
	const FVector2D DesignSize = mCombatHudReferenceSize;
	if (bSkin)
	{
		ApplyMobileSkinWidgetLayout();
	}

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

	RDUILayout::ApplyAnchoredSlot(mTurnChangeBackdropPanel, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), TurnChangeBackdropZOrder);
	RDUILayout::ApplyAnchoredSlot(mTurnChangeInputBlocker, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), TurnChangeInputZOrder);
	RDUILayout::ApplyAnchoredSlot(mTurnChangeVideoImage, FAnchors(0.140f, 0.155f, 0.860f, 0.845f), TurnChangeVideoZOrder);
	RDUILayout::ApplyAnchoredSlot(mTurnChangeTurnTextPanel, FAnchors(0.435f, 0.390f, 0.565f, 0.610f), TurnChangeTextZOrder);

	// 보유 주사위: 스크롤 없이 현재 개수를 도크 한 줄에 전부 맞춘다.
	if (mOwnedDiceDockBox != nullptr)
	{
		const int32 DiceCount = mOwnedDiceCellSizeBoxes.Num();
		float DockWidth = 0.0f;
		if (bSkin)
		{
			// 스킬 레일과 무관하게 하단 중앙에 배치한다. 8개까지는 144px 크기를 유지하고,
			// 그 이상일 때만 Safe Area 전체 폭 안에서 셀을 균등 축소한다.
			const float AvailableWidth = FMath::Max(1.0f,
				mCombatHudReferenceSize.X - MobileSafeInsetX * 2.0f);
			const float NaturalWidth = DiceCount > 0
				? StaticCast<float>(DiceCount) * MobileDiceCellSizePx
					+ StaticCast<float>(DiceCount - 1) * MobileDiceCellGapPx
				: AvailableWidth;
			DockWidth = FMath::Min(AvailableWidth, NaturalWidth);
			RDUILayout::ApplyDesignerSlotData(mOwnedDiceDockBox,
				MakePinnedSlot(FAnchors(0.5f, 1.0f),
					-DockWidth * 0.5f,
					-MobileBottomEdgeInsetPx - MobileDiceDockHeightPx,
					DockWidth, MobileDiceDockHeightPx), 22);
		}
		else
		{
			RDUILayout::ApplyAnchoredSlot(mOwnedDiceDockBox, FAnchors(0.23f, 0.82f, 0.755f, 0.955f), 22);
			if (RootCanvas != nullptr)
			{
				const float RootWidth = RootCanvas->GetCachedGeometry().GetLocalSize().X;
				if (RootWidth > 1.0f)
				{
					DockWidth = RootWidth * (0.755f - 0.23f);
				}
			}
		}

		if (DiceCount > 0)
		{
			// 8개까지는 12px 간격을 유지한다. 그 이상도 숨기지 않고 전체 간격을 도크의 10% 안으로 줄인다.
			const float DiceGap = DiceCount > 1
				? FMath::Min(MobileDiceCellGapPx, (DockWidth * 0.10f) / StaticCast<float>(DiceCount - 1))
				: 0.0f;
			const float CellSize = FMath::Min(MobileDiceCellSizePx,
				FMath::Max(1.0f, (DockWidth - DiceGap * StaticCast<float>(DiceCount - 1)) / StaticCast<float>(DiceCount)));
			for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
			{
				if (USizeBox* CellBox = mOwnedDiceCellSizeBoxes[DiceIndex])
				{
					CellBox->SetWidthOverride(CellSize);
					CellBox->SetHeightOverride(CellSize);
					if (UHorizontalBoxSlot* CellSlot = Cast<UHorizontalBoxSlot>(CellBox->Slot))
					{
						CellSlot->SetPadding(FMargin(0.0f, 0.0f,
							DiceIndex + 1 < DiceCount ? DiceGap : 0.0f, 0.0f));
					}
				}
			}
		}
	}

	// 이동 거리 힌트: 하단 주사위 도크 바로 위 중앙에 크게 표시한다.
	if (bSkin)
	{
		RDUILayout::ApplyDesignerSlotData(mDiceAssignmentText,
			MakePinnedSlot(FAnchors(0.5f, 1.0f), -240.0f,
				-MobileBottomEdgeInsetPx - MobileDiceDockHeightPx - MobileBottomGroupGapPx - 72.0f,
				480.0f, 64.0f), 24);
	}
	else
	{
		RDUILayout::ApplyAnchoredSlot(mDiceAssignmentText, FAnchors(0.34f, 0.745f, 0.66f, 0.81f), 24);
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
		const float DrawerWidth = FMath::Lerp(
			MobileSkillCollapsedWidthPx, MobileSkillExpandedWidthPx, mSkillDrawerExpansion);
		for (int32 SkillIndex = 0; SkillIndex < SkillRailPanelCount; ++SkillIndex)
		{
			const float RowTop = MobileSkillRailTopPx
				+ StaticCast<float>(SkillIndex) * (MobileSkillRowHeightPx + MobileSkillRowGapPx);
			const FAnchorData CellSlot = MakePointSlot(
				MobileSkillRailLeftPx, RowTop, DrawerWidth, MobileSkillRowHeightPx);
			RDUILayout::ApplyDesignerSlotData(mSkillRailPanels[SkillIndex], CellSlot, SkillRailZOrder);

			if (mSkillRailIcons.IsValidIndex(SkillIndex))
			{
				RDUILayout::ApplyDesignerSlotData(mSkillRailIcons[SkillIndex],
					MakePointSlot(
						MobileSkillRailLeftPx + (MobileSkillCollapsedWidthPx - MobileSkillIconSizePx) * 0.5f,
						RowTop + (MobileSkillRowHeightPx - MobileSkillIconSizePx) * 0.5f,
						MobileSkillIconSizePx, MobileSkillIconSizePx),
					SkillRailZOrder + 1);
			}
			if (mSkillRailTexts.IsValidIndex(SkillIndex))
			{
				const float NameWidth = FMath::Max(0.0f, DrawerWidth - MobileSkillCollapsedWidthPx - 16.0f);
				RDUILayout::ApplyDesignerSlotData(mSkillRailTexts[SkillIndex],
					MakePointSlot(MobileSkillRailLeftPx + MobileSkillCollapsedWidthPx,
						RowTop + 30.0f, NameWidth, 52.0f),
					SkillRailZOrder + 2);
			}
		}
		for (int32 SkillIndex = 0; SkillIndex < SkillInputCount; ++SkillIndex)
		{
			const float RowTop = MobileSkillRailTopPx
				+ StaticCast<float>(SkillIndex) * (MobileSkillRowHeightPx + MobileSkillRowGapPx);
			RDUILayout::ApplyDesignerSlotData(mSkillInputButtons[SkillIndex],
				MakePointSlot(MobileSkillRailLeftPx, RowTop, DrawerWidth, MobileSkillRowHeightPx),
				CombatSkillInputZOrder);
		}

		if (mSkillDrawerHandleButton != nullptr)
		{
			const float RailHeight = StaticCast<float>(CombatSkillSlotCount) * MobileSkillRowHeightPx
				+ StaticCast<float>(CombatSkillSlotCount - 1) * MobileSkillRowGapPx;
			RDUILayout::ApplyDesignerSlotData(mSkillDrawerHandleButton,
				MakePointSlot(
					MobileSkillRailLeftPx + DrawerWidth - 4.0f,
					MobileSkillRailTopPx + (RailHeight - MobileSkillDrawerHandleHeightPx) * 0.5f,
					MobileSkillDrawerHandleWidthPx, MobileSkillDrawerHandleHeightPx),
				CombatSkillInputZOrder + 1);
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
		// 모바일 핵심 입력은 WBP의 작은 마커를 복사하지 않고 현재 프로필 디자인 px로 고정한다.
		const FAnchors EndTurnAnchors(1.0f, 1.0f);
		const float EndTurnTopFromBottom = -MobileBottomEdgeInsetPx
			- MobileDiceDockHeightPx - MobileBottomGroupGapPx - MobileEndTurnHeightPx;
		RDUILayout::ApplyDesignerSlotData(EndTurnButton,
			MakePinnedSlot(EndTurnAnchors,
				-MobileSafeInsetX - MobileEndTurnWidthPx,
				EndTurnTopFromBottom,
				MobileEndTurnWidthPx, MobileEndTurnHeightPx), 18);
		RDUILayout::ApplyDesignerSlotData(mCancelActionButton,
			MakePinnedSlot(EndTurnAnchors,
				-MobileSafeInsetX - MobileEndTurnWidthPx - MobileActionButtonGapPx - MobileCancelActionWidthPx,
				EndTurnTopFromBottom,
				MobileCancelActionWidthPx, MobileEndTurnHeightPx), 18);

		constexpr float NavWidth = 120.0f;
		constexpr float NavHeight = 104.0f;
		constexpr float NavGap = 12.0f;
		auto ApplyNavButtonSlot = [&](UButton* Button, int32 IndexFromRight)
		{
			const int32 ColumnFromRight = mFoldVariantActive ? IndexFromRight % 2 : IndexFromRight;
			const int32 Row = mFoldVariantActive ? IndexFromRight / 2 : 0;
			const float RightOffset = MobileSafeInsetX + StaticCast<float>(ColumnFromRight) * (NavWidth + NavGap);
			RDUILayout::ApplyDesignerSlotData(Button,
				MakePinnedSlot(FAnchors(1.0f, 0.0f), -RightOffset - NavWidth,
					MobileSafeInsetTop + StaticCast<float>(Row) * (NavHeight + NavGap), NavWidth, NavHeight), 19);
		};
		ApplyNavButtonSlot(mNavSettingsButton, 0);
		ApplyNavButtonSlot(mNavSkillButton, 1);
		ApplyNavButtonSlot(mNavDiceButton, 2);
		ApplyNavButtonSlot(mNavMapButton, 3);
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
		RDUILayout::ApplyAnchoredSlot(EndTurnButton, FAnchors(0.79f, 0.82f, 0.965f, 0.965f), 18);
		RDUILayout::ApplyAnchoredSlot(mCancelActionButton, FAnchors(0.62f, 0.82f, 0.77f, 0.965f), 18);
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

void UCombatTileMapHUDWidget::ApplyMobileSkinWidgetLayout() const
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	auto SetSlot = [this](const TCHAR* WidgetName, const FAnchors& Anchors, const FVector2D& Position, const FVector2D& Size)
	{
		if (UWidget* Widget = WidgetTree->FindWidget(FName(WidgetName)))
		{
			if (UCanvasPanelSlot* Slot = RDUILayout::GetCanvasSlot(Widget))
			{
				Slot->SetAnchors(Anchors);
				Slot->SetAlignment(FVector2D::ZeroVector);
				Slot->SetAutoSize(false);
				Slot->SetPosition(Position);
				Slot->SetSize(Size);
			}
		}
	};

	auto HideWidget = [this](const TCHAR* WidgetName)
	{
		if (UWidget* Widget = WidgetTree->FindWidget(FName(WidgetName)))
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	// 결정된 모바일 HUD: 방 이름/턴 순서/별도 MOVE와 기존 96px 스킬 프레임은 사용하지 않는다.
	static const TCHAR* const HiddenWidgetNames[] = {
		TEXT("R_room_name_room_name_7"), TEXT("HUD_M_room_name_text"), TEXT("HUD_RoomNameFold"),
		TEXT("R_turn_order_turn_order_16"), TEXT("HUD_TurnOrder"), TEXT("HUD_M_turn_order_tokens"), TEXT("HUD_TurnOrderFold"),
		TEXT("R_btn_move_btn_move_29"), TEXT("HUD_Move"), TEXT("HUD_M_btn_move_label"),
		TEXT("HUD_SkillRail"), TEXT("HUD_DiceTray")
	};
	for (const TCHAR* WidgetName : HiddenWidgetNames)
	{
		HideWidget(WidgetName);
	}
	if (DesignCanvas != nullptr)
	{
		for (int32 ChildIndex = 0; ChildIndex < DesignCanvas->GetChildrenCount(); ++ChildIndex)
		{
			if (UWidget* Child = DesignCanvas->GetChildAt(ChildIndex);
				Child != nullptr && Child->GetName().StartsWith(TEXT("R_skill_rail_slot")))
			{
				Child->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// 상태 정보와 장비를 한 줄로 모아 제거된 중앙 상단 배너 공간을 전투 시야로 돌려준다.
	SetSlot(TEXT("R_top_status_bar____1"), FAnchors(0.0f), FVector2D(64.0f, 32.0f), FVector2D(150.0f, 96.0f));
	SetSlot(TEXT("R_top_status_bar_____1_2"), FAnchors(0.0f), FVector2D(80.0f, 56.0f), FVector2D(48.0f, 48.0f));
	SetSlot(TEXT("R_top_status_bar____3"), FAnchors(0.0f), FVector2D(226.0f, 32.0f), FVector2D(224.0f, 96.0f));
	SetSlot(TEXT("R_top_status_bar_____2_4"), FAnchors(0.0f), FVector2D(242.0f, 56.0f), FVector2D(48.0f, 48.0f));
	SetSlot(TEXT("R_top_status_bar____5"), FAnchors(0.0f), FVector2D(462.0f, 32.0f), FVector2D(320.0f, 96.0f));
	SetSlot(TEXT("R_top_status_bar_____3_6"), FAnchors(0.0f), FVector2D(478.0f, 52.0f), FVector2D(56.0f, 56.0f));
	SetSlot(TEXT("HUD_M_lv_value"), FAnchors(0.0f), FVector2D(112.0f, 58.0f), FVector2D(90.0f, 48.0f));
	SetSlot(TEXT("HUD_M_gold_value"), FAnchors(0.0f), FVector2D(292.0f, 58.0f), FVector2D(142.0f, 48.0f));
	SetSlot(TEXT("HUD_M_hp_value"), FAnchors(0.0f), FVector2D(552.0f, 58.0f), FVector2D(210.0f, 48.0f));

	static const TCHAR* const ValueTextNames[] = { TEXT("HUD_M_lv_value"), TEXT("HUD_M_gold_value"), TEXT("HUD_M_hp_value") };
	for (const TCHAR* TextName : ValueTextNames)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TextName))))
		{
			FSlateFontInfo Font = Text->GetFont();
			Font.Size = 34;
			Text->SetFont(Font);
			Text->SetJustification(ETextJustify::Center);
		}
	}

	static const TCHAR* const EquipNames[] = { TEXT("HUD_M_equip_0"), TEXT("HUD_M_equip_1"), TEXT("HUD_M_equip_2") };
	const float EquipSize = mFoldVariantActive ? 88.0f : 96.0f;
	const float EquipLeft = mFoldVariantActive ? 790.0f : 806.0f;
	const float EquipStep = mFoldVariantActive ? 100.0f : 108.0f;
	for (int32 EquipIndex = 0; EquipIndex < UE_ARRAY_COUNT(EquipNames); ++EquipIndex)
	{
		SetSlot(EquipNames[EquipIndex], FAnchors(0.0f),
			FVector2D(EquipLeft + StaticCast<float>(EquipIndex) * EquipStep, 32.0f), FVector2D(EquipSize, EquipSize));
	}

	// 상단 Nav: 프레임·아이콘·실제 입력 마커를 모두 같은 120x104 슬롯에 맞춘다.
	static const TCHAR* const NavFrames[] = {
		TEXT("R_btn_settings_btn_settings_14"), TEXT("R_btn_skill_btn_skill_12"),
		TEXT("R_btn_dice_btn_dice_10"), TEXT("R_btn_map_btn_map_8")
	};
	static const TCHAR* const NavIcons[] = {
		TEXT("R_btn_settings_______15"), TEXT("R_btn_skill_______13"),
		TEXT("R_btn_dice________11"), TEXT("R_btn_map______9")
	};
	static const TCHAR* const NavMarkers[] = { TEXT("HUD_Settings"), TEXT("HUD_Skill"), TEXT("HUD_Dice"), TEXT("HUD_Map") };
	constexpr float NavWidth = 120.0f;
	constexpr float NavHeight = 104.0f;
	constexpr float NavGap = 12.0f;
	for (int32 NavIndex = 0; NavIndex < UE_ARRAY_COUNT(NavFrames); ++NavIndex)
	{
		const int32 ColumnFromRight = mFoldVariantActive ? NavIndex % 2 : NavIndex;
		const int32 Row = mFoldVariantActive ? NavIndex / 2 : 0;
		const float RightOffset = MobileSafeInsetX + StaticCast<float>(ColumnFromRight) * (NavWidth + NavGap);
		const float NavTop = MobileSafeInsetTop + StaticCast<float>(Row) * (NavHeight + NavGap);
		const float FrameLeft = -RightOffset - NavWidth;
		SetSlot(NavFrames[NavIndex], FAnchors(1.0f, 0.0f), FVector2D(FrameLeft, NavTop), FVector2D(NavWidth, NavHeight));
		SetSlot(NavIcons[NavIndex], FAnchors(1.0f, 0.0f), FVector2D(FrameLeft + 24.0f, NavTop + 16.0f), FVector2D(72.0f, 72.0f));
		SetSlot(NavMarkers[NavIndex], FAnchors(1.0f, 0.0f), FVector2D(FrameLeft, NavTop), FVector2D(NavWidth, NavHeight));
	}

	// END TURN은 실제 Safe Area 우측 하단을 기준으로 주사위 도크 바로 위에 둔다.
	const FAnchors EndAnchors(1.0f, 1.0f);
	const float EndTurnTopFromBottom = -MobileBottomEdgeInsetPx
		- MobileDiceDockHeightPx - MobileBottomGroupGapPx - MobileEndTurnHeightPx;
	const FVector2D EndPosition(
		-MobileSafeInsetX - MobileEndTurnWidthPx,
		EndTurnTopFromBottom);
	SetSlot(TEXT("R_btn_end_turn_btn_end_turn_30"), EndAnchors, EndPosition,
		FVector2D(MobileEndTurnWidthPx, MobileEndTurnHeightPx));
	SetSlot(TEXT("HUD_EndTurn"), EndAnchors, EndPosition,
		FVector2D(MobileEndTurnWidthPx, MobileEndTurnHeightPx));
	SetSlot(TEXT("HUD_M_btn_end_turn_label"), EndAnchors,
		FVector2D(-MobileSafeInsetX - MobileEndTurnWidthPx,
			EndTurnTopFromBottom + 16.0f),
		FVector2D(MobileEndTurnWidthPx, 112.0f));
	if (UTextBlock* EndText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("HUD_M_btn_end_turn_label"))))
	{
		FSlateFontInfo EndFont = EndText->GetFont();
		EndFont.Size = 36;
		EndText->SetFont(EndFont);
		EndText->SetJustification(ETextJustify::Center);
	}
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
		TEXT("CombatHUD LayoutMetrics: viewport=%.1f,%.1f aspect=%.3f skin=%s designRef=%.1f,%.1f designCanvas=%.1f,%.1f rootCanvas=%.1f,%.1f skillRail={%s %.3f,%.3f..%.3f,%.3f}"),
		ViewportSize.X, ViewportSize.Y, Aspect,
		IsDesignerSkinActive() ? TEXT("on") : TEXT("off"),
		mCombatHudReferenceSize.X, mCombatHudReferenceSize.Y,
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
	if (ViewportSize.X <= 1.0f || ViewportSize.Y <= 1.0f)
	{
		return;
	}
	if (IsDesignerSkinActive() == false)
	{
		// 레거시 정규화 도크도 실제 RootCanvas 폭이 생긴 첫 Tick에 셀 크기를 다시 맞춘다.
		ApplyRuntimeWidgetLayout();
		return;
	}

	// 좁은 화면에서는 상태 버튼을 2x2로 접고 1440 최소 논리폭을 유지한다.
	// 화면이 4:3보다 좁으면 높이를 늘려 기준 캔버스의 종횡비를 실제 Safe Area와 정확히 맞춘다.
	// 따라서 ScaleToFit이 더 이상 위/아래 레터박스를 만들지 않으며, 하단 앵커도 실제 화면 끝을 따른다.
	constexpr float FoldAspectCut = 1.68f;
	const float Aspect = ViewportSize.X / ViewportSize.Y;
	const bool bFold = Aspect < FoldAspectCut;
	mFoldVariantActive = bFold;
	const float MinimumReferenceWidth = bFold ? MobileCompactReferenceWidthPx : MobileWideReferenceWidthPx;
	const float ReferenceWidth = FMath::Max(MinimumReferenceWidth, Aspect * MobileReferenceHeightPx);
	const float ReferenceHeight = ReferenceWidth / Aspect;
	mCombatHudReferenceSize = FVector2D(ReferenceWidth, ReferenceHeight);
	if (mCombatDesignSizeBox != nullptr)
	{
		mCombatDesignSizeBox->SetWidthOverride(mCombatHudReferenceSize.X);
		mCombatDesignSizeBox->SetHeightOverride(mCombatHudReferenceSize.Y);
	}
	// 프로필과 기준 폭을 바꾼 같은 프레임에 모든 런타임/스킨 슬롯을 다시 계산한다.
	ApplyRuntimeWidgetLayout();
}
