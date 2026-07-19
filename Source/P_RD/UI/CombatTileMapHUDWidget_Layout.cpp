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
	auto ApplyScreenRect = [bSkin, &DesignSize](UWidget* Widget, const FAnchors& Rect, int32 ZOrder)
	{
		if (bSkin)
		{
			RDUILayout::ApplyDesignerSlotData(Widget, RDUILayout::NormalizedToDesignPointSlot(Rect, DesignSize), ZOrder);
		}
		else
		{
			RDUILayout::ApplyAnchoredSlot(Widget, Rect, ZOrder);
		}
	};

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

	// 주사위는 별도 배치 도구가 아니라 직접 조작의 자동 소모 자원이다. 전장을 가리지 않는 하단 벨트로 압축한다.
	const int32 OwnedDiceCount = mOwnedDiceImages.Num();
	ApplyScreenRect(mDiceTrayPanel, FAnchors(0.225f, 0.882f, 0.705f, 0.988f), 16);
	ApplyScreenRect(mDiceTrayTitleText, FAnchors(0.238f, 0.892f, 0.335f, 0.918f), 18);
	ApplyScreenRect(mDiceAssignmentText, FAnchors(0.340f, 0.892f, 0.690f, 0.918f), 18);
	if (OwnedDiceCount > 0)
	{
		const float StartLeft = 0.242f;
		const float EndRight = 0.690f;
		const float Gap = 0.006f;
		const float CellWidth = (EndRight - StartLeft - Gap * StaticCast<float>(FMath::Max(OwnedDiceCount - 1, 0)))
			/ StaticCast<float>(OwnedDiceCount);
		for (int32 DiceIndex = 0; DiceIndex < OwnedDiceCount; ++DiceIndex)
		{
			const float CellLeft = StartLeft + StaticCast<float>(DiceIndex) * (CellWidth + Gap);
			const float CellRight = CellLeft + CellWidth;
			ApplyScreenRect(mOwnedDiceImages[DiceIndex], FAnchors(CellLeft + CellWidth * 0.20f, 0.918f, CellRight - CellWidth * 0.20f, 0.967f), 22);
			if (mOwnedDiceCardWidgets.IsValidIndex(DiceIndex))
			{
				ApplyScreenRect(mOwnedDiceCardWidgets[DiceIndex], FAnchors(CellLeft, 0.916f, CellRight, 0.980f), 23);
			}
			if (mOwnedDiceTypeTexts.IsValidIndex(DiceIndex))
			{
				if (mOwnedDiceTypeTexts[DiceIndex] != nullptr)
				{
					ApplyScreenRect(mOwnedDiceTypeTexts[DiceIndex], FAnchors(CellLeft, 0.963f, CellRight, 0.982f), 24);
				}
			}
		}
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
	// 레거시 스킬 레일은 호환용 객체만 남기고 표시/입력하지 않는다. 행동은 컨텍스트 팔레트가 소유한다.
	const int32 SkillRailZOrder = IsSkillDetailVisible() ? CombatSkillDetailRailZOrder : 18;
	const int32 SkillRailPanelCount = mSkillRailPanels.Num();
	const int32 SkillInputCount = mSkillInputButtons.Num();
	ApplyScreenRect(mSkillDockPanel, FAnchors(0.012f, 0.175f, 0.212f, 0.830f), SkillRailZOrder - 2);
	ApplyScreenRect(mSkillDockTitleText, FAnchors(0.022f, 0.184f, 0.202f, 0.218f), SkillRailZOrder + 3);
	for (int32 SkillIndex = 0; SkillIndex < SkillRailPanelCount; ++SkillIndex)
	{
		const FAnchors ItemRect = GetSkillRailItemRect(SkillIndex, SkillRailPanelCount);
		ApplyScreenRect(mSkillRailPanels[SkillIndex], ItemRect, SkillRailZOrder);
		const float ItemW = ItemRect.Maximum.X - ItemRect.Minimum.X;
		const float ItemH = ItemRect.Maximum.Y - ItemRect.Minimum.Y;
		if (mSkillRailIcons.IsValidIndex(SkillIndex))
		{
			const FAnchors IconRect(
				ItemRect.Minimum.X + ItemW * 0.035f,
				ItemRect.Minimum.Y + ItemH * 0.10f,
				ItemRect.Minimum.X + ItemW * 0.285f,
				ItemRect.Maximum.Y - ItemH * 0.10f);
			ApplyScreenRect(mSkillRailIcons[SkillIndex], IconRect, SkillRailZOrder + 1);
		}
		if (mSkillRailTexts.IsValidIndex(SkillIndex))
		{
			const FAnchors TextRect(
				ItemRect.Minimum.X + ItemW * 0.315f,
				ItemRect.Minimum.Y + ItemH * 0.10f,
				ItemRect.Maximum.X - ItemW * 0.035f,
				ItemRect.Maximum.Y - ItemH * 0.08f);
			ApplyScreenRect(mSkillRailTexts[SkillIndex], TextRect, SkillRailZOrder + 2);
		}
	}
	for (int32 SkillIndex = 0; SkillIndex < SkillInputCount; ++SkillIndex)
	{
		ApplyScreenRect(mSkillInputButtons[SkillIndex], GetSkillRailItemRect(SkillIndex, SkillInputCount), CombatSkillInputZOrder);
	}
	// 디자이너 WBP에 구 레일/하단 트레이 프레임이 구워져 있어도 새 런타임 UI 뒤에 잔상으로 남기지 않는다.
	if (WidgetTree != nullptr)
	{
		if (UWidget* LegacySkillRail = WidgetTree->FindWidget(TEXT("HUD_SkillRail")))
		{
			LegacySkillRail->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (UWidget* LegacyDiceTray = WidgetTree->FindWidget(TEXT("HUD_DiceTray")))
		{
			LegacyDiceTray->SetVisibility(ESlateVisibility::Collapsed);
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

	// 코치마크는 한 문장만 담는 작은 카드로 유지해 전장과 적 계획판을 덮지 않는다.
	ApplyScreenRect(mEnemyIntentPanel, FAnchors(0.690f, 0.150f, 0.940f, 0.355f), 36);
	ApplyScreenRect(mEnemyIntentTutorialPanel, FAnchors(0.340f, 0.040f, 0.660f, 0.175f), 220);
	// 주사위 패널 바로 위의 작은 확정 카드. 전장 중앙과 우측 명령 버튼을 피한다.
	ApplyScreenRect(mDisplacementConfirmPanel, FAnchors(0.365f, 0.685f, 0.665f, 0.805f), 920);

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
	// 카드형 도크는 WBP의 좁은 아이콘 마커를 상속하지 않는다. 이름/역할/비용과 입력영역이
	// 모든 화면비에서 같은 정규화 rect를 공유하도록 HUD가 직접 소유한다.
	return FAnchors(0.018f, 0.225f, 0.205f, 0.820f);
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
