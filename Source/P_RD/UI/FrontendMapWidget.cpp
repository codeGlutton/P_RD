#include "UI/FrontendMapWidget.h"
#include "UI/RunOptionsRailWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameMode/RoomGameModeBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Styling/SlateBrush.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/ViewportZOrderType.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/*
	 * 아래 수치들은 시안 마커(Map_NodeArea/Map_NodeMetrics/Map_ColPitch)가 WBP에 없을 때만 쓰는 폴백이다.
	 * 배치 경계/간격의 정본은 concept_worldmap_claude02.json -> 빌더 -> WBP 마커다. C++ 하드코딩 금지.
	 */
	constexpr float MapGraphFallbackWidth = 1280.f;
	constexpr float MapGraphFallbackAspect = 1.5f;
	constexpr float MapNodeFallbackSize = 96.f;
	constexpr float MapRowPitchFallback = 176.f;
	constexpr float MapColPitchMaxFallback = 240.f;
	constexpr float MapNodeAreaLeftFracFallback = 0.10f;
	constexpr float MapNodeAreaRightFracFallback = 0.90f;
	constexpr float MapNodeAreaTopPxFallback = 96.f;
	constexpr float MapNodeAreaBottomPxFallback = 168.f;
	constexpr float MapNodeAreaNarrowLeftSafeFrac = 0.14f;
	constexpr float MapNodeAreaMinLeftSafePx = 120.f;
	constexpr float MapNodeAreaMaxLeftSafePx = 190.f;
	constexpr float MapNodeAreaNarrowRightInsetFrac = 0.055f;
	constexpr float MapNodeAreaMinRightInsetPx = 48.f;
	constexpr float MapNodeAreaMaxRightInsetPx = 96.f;
	constexpr float MapNodeAreaMinUsableWidthPx = 260.f;
	constexpr float MapNodeLegendGapPx = 36.f;

	/*
	 * 시안 마커(0.22~0.80)는 1920 가로 화면 + 책상 장식 기준으로 잡힌 값이다.
	 * 책상 배경은 폐기했고, 폰 세로 화면에서 그 여백을 그대로 두면 노드가 놓일
	 * 폭이 화면의 58%로 줄어 열 간격이 아이콘보다 좁아진다(실측 42.5px vs 162px).
	 * 좁은 화면에서는 마커보다 넓게 펴서 열 간격을 확보한다.
	 */
	constexpr float MapNodeAreaMinSpanFrac = 0.92f;
	constexpr float MapNodeAreaEdgeInsetPx = 10.f;

	/*
	 * 아이콘 크기는 열 간격에서 파생한다. 둘을 따로 정하면 화면이 좁아질 때
	 * 반드시 겹친다 -- 열 간격에는 상한만 있고 아이콘 크기와 묶인 하한이 없었다.
	 * 한 열이 차지하는 칸의 이 비율만큼만 아이콘이 차지하게 해 겹침을 원천 차단한다.
	 */
	constexpr float MapNodeSlotFillRatio = 0.82f;
	constexpr float MapNodeMinSizePx = 56.f;

	/** @brief 연결선 두께 = 노드 지름의 이 비율(시안 두께가 하한). 노드가 커지면 선도 굵어진다. */
	constexpr float MapLineThicknessNodeRatio = 0.11f;

	/*
	 * 범례 판 크기. 시안이 준 고정 크기(429x599)는 1920 가로 기준이라 폰에서
	 * 글자가 작다. 화면 폭 기준으로 잡되, 짧은 화면에서 세로를 다 덮지 않도록
	 * 높이에 상한을 둔다.
	 */
	// 범례는 단추로 여닫으므로 펼쳤을 때는 시원하게 크게 보여준다(가릴 걱정이 없다).
	constexpr float MapLegendWidthFrac = 0.42f;
	constexpr float MapLegendMaxHeightFrac = 0.72f;

	/** @brief 범례/단추를 화면 가장자리에서 띄우는 여백(화면 폭 비율). */
	constexpr float MapLegendEdgeMarginFrac = 0.025f;

	/**
	 * @brief BACK 단추 폭(화면 폭 비율)과 가로세로 비율.
	 *
	 * 판을 9-slice 로 그리게 되면서 이 비율은 더 이상 그림이 강제하는 값이 아니다.
	 * 단추가 알약처럼 보이게 하는 모양 선택일 뿐이라 자유롭게 바꿔도 된다.
	 */
	constexpr float MapCloseButtonWidthFrac = 0.24f;
	constexpr float MapCloseButtonAspect = 2.508f;
	constexpr float MapCloseButtonMinWidthPx = 220.f;

	/** @brief 열 간격 상한이 아이콘보다 좁아지지 않게 하는 최소 배수(아이콘 지름 대비). */
	constexpr float MapColPitchMinNodeRatio = 1.2f;

	/** @brief 방마다 붙은 -1~1 흔들림의 최대 진폭(px). 남은 여유 안에서만 흔든다. */
	constexpr float MapNodeJitterMaxPx = 18.f;

	/*
	 * 지도 팝업이 차지하는 세로 밴드(화면 비율). 위아래로만 배경을 살짝 보여
	 * "떠 있는 팝업"으로 읽히게 한다. 가로는 폰에서 지도를 최대한 크게 쓰려고
	 * 꽉 채우며, 원근 사다리꼴이 위쪽 좌우를 알아서 비운다.
	 */
	constexpr float MapPopupBandTopFrac = 0.035f;
	constexpr float MapPopupBandBottomFrac = 0.965f;

	const FLinearColor PanelDarkColor(0.215f, 0.240f, 0.260f, 1.f);
	const FLinearColor AccentFillColor(0.255f, 0.565f, 0.590f, 1.f);
	const FLinearColor SelectFillColor(0.640f, 0.545f, 0.345f, 1.f);
	const FSlateColor TitleColor(FLinearColor(0.94f, 0.95f, 0.93f, 1.f));
	const FSlateColor MutedColor(FLinearColor(0.63f, 0.67f, 0.69f, 1.f));
	const FSlateColor ReadyColor(FLinearColor(0.53f, 0.86f, 0.88f, 1.f));
	const FSlateColor LockedColor(FLinearColor(0.52f, 0.54f, 0.55f, 1.f));
	const FLinearColor TransparentPanelColor(0.f, 0.f, 0.f, 0.f);

	float GetFrontendMapLegendScale(float GraphWidth, float LegendRefWidth, float LegendMinScale)
	{
		return LegendRefWidth > KINDA_SMALL_NUMBER
			? FMath::Clamp(GraphWidth / LegendRefWidth, LegendMinScale, 1.f)
			: 1.f;
	}

	FText FrontendMapText(const TCHAR* Key)
	{
		/*
		 * 지도 위젯에서 쓰는 기본 문구를 한 곳에서 관리한다.
		 * WBP는 레이아웃/스타일을 담당하고, C++은 상태별 의미 문구를 내려준다.
		 * 최종 로컬라이징 테이블이 생기기 전까지는 NSLOCTEXT 키를 이 helper에 모아둔다.
		 */
		if (FCString::Strcmp(Key, TEXT("MapText")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapText", "MAP");
		}
		if (FCString::Strcmp(Key, TEXT("CloseText")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "CloseText", "BACK");
		}
		if (FCString::Strcmp(Key, TEXT("EnterText")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "EnterText", "ENTER");
		}
		if (FCString::Strcmp(Key, TEXT("LegendText")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "LegendText", "LEGEND");
		}
		if (FCString::Strcmp(Key, TEXT("LoadingStatusText")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "LoadingStatusText", "Loading");
		}
		if (FCString::Strcmp(Key, TEXT("MapUnavailableStatusText")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapUnavailableStatusText", "Map is not ready");
		}
		if (FCString::Strcmp(Key, TEXT("MapStartBadge")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapStartBadge", "START");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomSelectedBadge")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomSelectedBadge", "SELECT");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomReadyBadge")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomReadyBadge", "READY");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomVisitedBadge")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomVisitedBadge", "DONE");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomLockedBadge")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomLockedBadge", "LOCK");
		}
		if (FCString::Strcmp(Key, TEXT("MapStartNodeLabel")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapStartNodeLabel", "START");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomCompactNodeLabelFormat")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomCompactNodeLabelFormat", "{0}-{1}");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomDebugNodeLabelFormat")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomDebugNodeLabelFormat", "{0}-{1} {2}");
		}
	if (FCString::Strcmp(Key, TEXT("MapStartState")) == 0)
	{
		return NSLOCTEXT("FrontendMapWidget", "MapStartState", "Start point");
	}
	if (FCString::Strcmp(Key, TEXT("StartPointTitle")) == 0)
	{
		return NSLOCTEXT("FrontendMapWidget", "StartPointTitle", "Start");
	}
	if (FCString::Strcmp(Key, TEXT("MapRoomSelected")) == 0)
	{
		return NSLOCTEXT("FrontendMapWidget", "MapRoomSelected", "Selected");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomReady")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomReady", "Ready");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomVisited")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomVisited", "Visited");
		}
		if (FCString::Strcmp(Key, TEXT("MapRoomLocked")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "MapRoomLocked", "Locked");
		}
		if (FCString::Strcmp(Key, TEXT("NoRoomSelected")) == 0)
		{
			return NSLOCTEXT("FrontendMapWidget", "NoRoomSelected", "Select a room");
		}
		return FText::FromString(Key);
	}

	FLinearColor GetMapRoomTypeColor(ERoomType RoomType)
	{
		/*
		 * RoomType별 임시 색상이다.
		 * 최종 아이콘/이미지가 붙기 전까지 APK에서 룸 타입 구분이 맞는지 빠르게 확인하기 위한 표시 규칙이다.
		 */
		switch (RoomType)
		{
		case ERoomType::Monster:
			return FLinearColor(0.210f, 0.430f, 0.470f, 1.f);
		case ERoomType::EliteMonster:
			return FLinearColor(0.550f, 0.355f, 0.190f, 1.f);
		case ERoomType::BossMonster:
			return FLinearColor(0.540f, 0.140f, 0.145f, 1.f);
		case ERoomType::Shop:
			return FLinearColor(0.205f, 0.390f, 0.245f, 1.f);
		case ERoomType::Treasure:
			return FLinearColor(0.590f, 0.460f, 0.180f, 1.f);
		default:
			return FLinearColor(0.250f, 0.285f, 0.305f, 1.f);
		}
	}

	FLinearColor GetMapRoomPanelColor(const FMapRoomView& Room)
	{
		/*
		 * 방의 게임 상태를 지도 노드의 배경색으로 바꾼다.
		 * UI 위젯은 Ready/Selected/Cleared/Locked 같은 View 상태만 보고 색을 고르며,
		 * 실제 선택 가능 여부 판단은 GameMode에서 끝난 뒤 DTO로 내려온다.
		 */
		if (Room.mIsStartPoint)
		{
			return FLinearColor(0.245f, 0.300f, 0.315f, 1.f);
		}

		switch (Room.mState)
		{
		case EMapRoomState::Selected:
			return SelectFillColor;
		case EMapRoomState::Ready:
			return AccentFillColor;
		case EMapRoomState::Cleared:
			return FLinearColor(0.180f, 0.260f, 0.210f, 1.f);
		default:
			return PanelDarkColor;
		}
	}

	FSlateColor GetMapRoomTextColor(const FMapRoomView& Room)
	{
		/* 잠긴 노드는 텍스트도 흐리게 보여 "보이지만 아직 갈 수 없음"을 명확히 한다. */
		return Room.mState == EMapRoomState::Locked ? LockedColor : TitleColor;
	}

	FText GetMapRoomBadgeText(const FMapRoomView& Room)
	{
		/*
		 * 텍스트형 지도 UI에서 쓰던 상태 배지 문구다.
		 * 현재 WBP에서는 일부 텍스트 표면을 숨기지만, 지도 노드/프리뷰 표현이 다시 필요해질 때 같은 상태 문구를 재사용한다.
		 */
		if (Room.mIsStartPoint)
		{
			return FrontendMapText(TEXT("MapStartBadge"));
		}

		switch (Room.mState)
		{
		case EMapRoomState::Selected:
			return FrontendMapText(TEXT("MapRoomSelectedBadge"));
		case EMapRoomState::Ready:
			return FrontendMapText(TEXT("MapRoomReadyBadge"));
		case EMapRoomState::Cleared:
			return FrontendMapText(TEXT("MapRoomVisitedBadge"));
		default:
			return FrontendMapText(TEXT("MapRoomLockedBadge"));
		}
	}

	FText GetMapRoomNodeLabel(const FMapRoomView& Room)
	{
		/*
		 * 최종 아이콘이 없을 때 노드 안에 표시할 최소 라벨이다.
		 * 시작점은 START, 일반 방은 행-열 형식으로 표시해 생성된 Stage 구조를 확인한다.
		 */
		if (Room.mIsStartPoint)
		{
			return FrontendMapText(TEXT("MapStartNodeLabel"));
		}

		return FText::Format(
			FrontendMapText(TEXT("MapRoomCompactNodeLabelFormat")),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1));
	}

	FText GetMapRoomStateText(const FMapRoomView& Room)
	{
		/*
		 * 프리뷰 영역이나 접근성 텍스트에서 사용할 상태 문구다.
		 * 노드 클릭 가능 여부와 별개로, 사용자가 현재 방이 왜 어둡거나 밝은지 읽을 수 있게 한다.
		 */
		if (Room.mIsStartPoint)
		{
			return FrontendMapText(TEXT("MapStartState"));
		}

		switch (Room.mState)
		{
		case EMapRoomState::Selected:
			return FrontendMapText(TEXT("MapRoomSelected"));
		case EMapRoomState::Ready:
			return FrontendMapText(TEXT("MapRoomReady"));
		case EMapRoomState::Cleared:
			return FrontendMapText(TEXT("MapRoomVisited"));
		default:
			return FrontendMapText(TEXT("MapRoomLocked"));
		}
	}

	FText GetRoomDebugTypeText(ERoomType RoomType)
	{
		/*
		 * 임시 디버그 노드 라벨에 들어갈 룸 타입 약어다.
		 * 최종 WBP에서 룸 타입별 아이콘이 들어가면 이 텍스트는 숨기거나 제거할 수 있다.
		 */
		switch (RoomType)
		{
		case ERoomType::Monster:
			return NSLOCTEXT("FrontendMapWidget", "DebugMonsterRoomType", "MON");
		case ERoomType::EliteMonster:
			return NSLOCTEXT("FrontendMapWidget", "DebugEliteRoomType", "ELITE");
		case ERoomType::BossMonster:
			return NSLOCTEXT("FrontendMapWidget", "DebugBossRoomType", "BOSS");
		case ERoomType::Shop:
			return NSLOCTEXT("FrontendMapWidget", "DebugShopRoomType", "SHOP");
		case ERoomType::Treasure:
			return NSLOCTEXT("FrontendMapWidget", "DebugTreasureRoomType", "TRS");
		default:
			return NSLOCTEXT("FrontendMapWidget", "DebugStartRoomType", "START");
		}
	}

	/**
	 * @brief 현재 지도 노드에 임시로 표시할 디버그 라벨을 만든다.
	 *
	 * @details
	 * 최종 룸 노드는 RoomType별 아이콘/이미지를 표시해야 한다. 지금은 아이콘 리소스와 최종 룸 표시 규칙이
	 * 확정되기 전이라, APK에서 Stage 행/열과 RoomType 매핑이 맞는지만 확인할 수 있도록 텍스트를 남긴다.
	 */
	FText GetMapRoomDebugNodeLabel(const FMapRoomView& Room)
	{
		return FText::Format(
			FrontendMapText(TEXT("MapRoomDebugNodeLabelFormat")),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1),
			GetRoomDebugTypeText(Room.mType));
	}

	const FMapRoomView* FindMapRoom(const TArray<FMapRoomView>& Rooms, int32 RowIndex, int32 ColumnIndex)
	{
		/*
		 * 선을 그릴 때 "현재 방의 다음 열"이 실제 방 View 배열 안에 있는지 찾는다.
		 * FStage 내부 구조를 지도 위젯이 직접 들고 있지 않기 때문에, DTO 배열 안에서 row/column으로만 연결한다.
		 */
		return Rooms.FindByPredicate([RowIndex, ColumnIndex](const FMapRoomView& Room)
		{
			return Room.mRow == RowIndex && Room.mColumn == ColumnIndex;
		});
	}
}

/**
 * @brief 월드맵 팝업 ZOrder와 기본 문구 캐시를 초기화한다.
 *
 * @details
 * 선/노드 WBP 클래스는 WBP_FrontendMap Class Defaults에서 지정한다.
 * C++은 특정 WBP 경로를 직접 알지 않고, 런타임에는 지정된 클래스가 없을 때 경고만 남긴다.
 * 월드맵은 탑바 조회와 승리 후 선택 흐름 모두에서 HUD보다 위인 전체 화면 팝업으로 표시한다.
 */
UFrontendMapWidget::UFrontendMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 조회용 탑바 지도와 승리 후 방 선택 지도가 같은 전체 화면 위젯을 쓴다.
	// HUD 아래에 두면 전투 중 열었을 때 스킬 카드와 턴 UI가 지도 위를 덮으므로
	// 인벤토리/설정과 같은 팝업 레이어에서 입력과 표시를 독점한다.
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);

	// 원근은 이미지에 굽지 않는다 — 평평한 지도 + 리테이너 머티리얼이 정본.
	static ConstructorHelpers::FObjectFinder<UTexture2D> MapParchmentFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Map/T_StageMap_Background_Parchment.T_StageMap_Background_Parchment"));
	if (MapParchmentFinder.Succeeded())
	{
		mMapParchmentTexture = MapParchmentFinder.Object;
	}

	/*
	 * 지도 뒷배경과 범례 그림은 지웠다. 새 그림으로 갈 예정이라 자리만 비운다.
	 * 여기서 못 찾은 경로를 남겨 두면 CDO 를 만들 때마다 로그가 찍히므로
	 * FObjectFinder 자체를 걷는다. ApplyCurrentMapArt()/UpdateLegendPlateLayout()
	 * 은 이미 nullptr 을 검사하고 조용히 건너뛴다.
	 */

	static ConstructorHelpers::FObjectFinder<UTexture2D> MapButtonPlateFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/Common/T_KitA_Button_Wide_Normal.T_KitA_Button_Wide_Normal"));
	if (MapButtonPlateFinder.Succeeded())
	{
		mMapButtonPlateTexture = MapButtonPlateFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MapPerspectiveFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/RunFlow/M_MapPerspective.M_MapPerspective"));
	if (MapPerspectiveFinder.Succeeded())
	{
		mMapPerspectiveMaterial = MapPerspectiveFinder.Object;
	}

	// 범례 아이콘 v2 교체용. 키워드는 옛 텍스처 경로에서 종류를 찾을 때 쓴다.
	// monster 는 corruptedmonster 와도 겹치므로 마지막에 검사한다(ApplyLegendIconsV2).
	static const struct { const TCHAR* Keyword; const TCHAR* Path; } IconV2Entries[] = {
		{ TEXT("elite"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_MapNode_Elite_V2.T_MapNode_Elite_V2") },
		{ TEXT("boss"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_MapNode_Boss_V2.T_MapNode_Boss_V2") },
		{ TEXT("shop"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_MapNode_Shop_V2.T_MapNode_Shop_V2") },
		{ TEXT("treasure"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_MapNode_Treasure_V2.T_MapNode_Treasure_V2") },
		{ TEXT("rest"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_MapNode_Rest_V2.T_MapNode_Rest_V2") },
		{ TEXT("camp"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_MapNode_Rest_V2.T_MapNode_Rest_V2") },
		{ TEXT("monster"), TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/T_MapNode_Monster_V2.T_MapNode_Monster_V2") },
	};
	for (const auto& Entry : IconV2Entries)
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> IconFinder(Entry.Path);
		if (IconFinder.Succeeded())
		{
			mMapIconV2ByKeyword.Add(FString(Entry.Keyword), IconFinder.Object);
		}
	}

	RefreshLocalizedTextCache();
}

/**
 * @brief 디자이너 바인딩과 버튼 이벤트를 준비하고 현재 지도 데이터를 그린다.
 */
void UFrontendMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// 설정바는 지도 WidgetTree의 자식이 아니라 별도 Viewport 위젯이다. 지도만
	// CloseUI로 접으면 설정바가 화면에 남아 아래 전투 HUD의 MAP 버튼을 덮는다.
	OnNativeVisibilityChanged.RemoveAll(this);
	OnNativeVisibilityChanged.AddUObject(
		this, &UFrontendMapWidget::HandleMapVisibilityChanged);
	EnsureRunOptionsRail();
	if (IsLandscapeLayout())
	{
		EnsureCloseButton();
		ValidateDesignerBindings();
		BindEvents();
		ConfigureMapGraphLayout();
		RefreshLocalizedTextCache();
		HideUnusedMapTextSurfaces();
		RefreshMap();
		return;
	}

	EnsureCloseButton();
	ValidateDesignerBindings();
	BindEvents();
	InstallMapPerspectiveRetainer();
	EnsureParchmentVeil();
	EnsureLegendPlate();
	EnsureLegendToggleButton();
	ApplyLegendShownState();
	ApplyCurrentMapArt();
	ApplyLegendIconsV2();

	ConfigureMapGraphLayout();
	RefreshLocalizedTextCache();
	HideUnusedMapTextSurfaces();
	RefreshMap();
}

void UFrontendMapWidget::EnsureRunOptionsRail()
{
	if (mRunOptionsRailWidget != nullptr)
	{
		mRunOptionsRailWidget->SetMapContext(true);
		SyncRunOptionsRailVisibility(GetVisibility());
		return;
	}
	UClass* RailClass = LoadClass<URunOptionsRailWidget>(nullptr,
		TEXT("/Game/UI/Common/WBP_RunOptionsRail.WBP_RunOptionsRail_C"));
	if (RailClass == nullptr)
	{
		RailClass = URunOptionsRailWidget::StaticClass();
	}
	if (APlayerController* Owner = GetOwningPlayer())
	{
		mRunOptionsRailWidget = CreateWidget<URunOptionsRailWidget>(Owner, RailClass);
	}
	else if (UWorld* World = GetWorld())
	{
		mRunOptionsRailWidget = CreateWidget<URunOptionsRailWidget>(World, RailClass);
	}
	if (mRunOptionsRailWidget != nullptr)
	{
		mRunOptionsRailWidget->SetMapContext(true);
		if (GetOwningPlayer() != nullptr)
		{
			mRunOptionsRailWidget->AddToViewport(10001);
		}
		SyncRunOptionsRailVisibility(GetVisibility());
	}
}

void UFrontendMapWidget::HandleMapVisibilityChanged(
	const ESlateVisibility InVisibility)
{
	SyncRunOptionsRailVisibility(InVisibility);
}

void UFrontendMapWidget::SyncRunOptionsRailVisibility(
	const ESlateVisibility MapVisibility) const
{
	if (mRunOptionsRailWidget == nullptr)
	{
		return;
	}

	const bool bMapShown = MapVisibility != ESlateVisibility::Collapsed
		&& MapVisibility != ESlateVisibility::Hidden;
	mRunOptionsRailWidget->SetVisibility(bMapShown
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

/**
 * @brief 디자이너 자산에 닫기 컨트롤이 없을 때만 런타임 BACK 버튼을 보완한다.
 */
void UFrontendMapWidget::EnsureCloseButton()
{
	if (CloseButton != nullptr || WidgetTree == nullptr)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(
		WidgetTree->FindWidget(TEXT("FrontendMapRoot")));
	if (RootCanvas == nullptr)
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	if (RootCanvas == nullptr)
	{
		UE_LOG(LogRD, Warning,
			TEXT("FrontendMapWidget: runtime CloseButton requires a CanvasPanel root."));
		return;
	}

	CloseButton = Cast<UButton>(
		WidgetTree->FindWidget(TEXT("RuntimeCloseButton")));
	if (CloseButton == nullptr)
	{
		CloseButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("RuntimeCloseButton"));
	}
	if (CloseButton == nullptr)
	{
		return;
	}

	if (mMapButtonPlateTexture != nullptr)
	{
		/*
		 * 공용 단추 판(KitA). 지도만 다른 단추를 쓰던 것을 다른 화면과 같은
		 * 그림으로 맞췄다 -- 같은 기능인데 지도·탭·설정이 서로 다른 단추를
		 * 쓰고 있었다(0804 검수).
		 *
		 * 이 그림은 9-slice 로 그린다. 모서리 장식 45px, 테두리 44px 을 실측해
		 * 뒀고(Saved/UIKit/ConceptA/_nineslice.txt), 잘라낼 때 남긴 투명 여백
		 * 8px 을 더한 자리가 마진이다. 그래서 폭을 늘려도 모서리가 안 늘어난다.
		 */
		const FVector2D PlateSize(mMapButtonPlateTexture->GetSizeX(),
			mMapButtonPlateTexture->GetSizeY());
		FSlateBrush PlateBrush;
		PlateBrush.DrawAs = ESlateBrushDrawType::Box;
		PlateBrush.Margin = FMargin(
			(8.f + 45.f) / PlateSize.X, (8.f + 44.f) / PlateSize.Y,
			(8.f + 45.f) / PlateSize.X, (8.f + 44.f) / PlateSize.Y);
		PlateBrush.SetResourceObject(mMapButtonPlateTexture);
		PlateBrush.ImageSize = PlateSize;

		FButtonStyle PlateStyle;
		PlateStyle.SetNormal(PlateBrush);
		FSlateBrush HoveredBrush = PlateBrush;
		HoveredBrush.TintColor = FSlateColor(FLinearColor(1.12f, 1.12f, 1.08f, 1.f));
		PlateStyle.SetHovered(HoveredBrush);
		FSlateBrush PressedBrush = PlateBrush;
		PressedBrush.TintColor = FSlateColor(FLinearColor(0.78f, 0.78f, 0.76f, 1.f));
		PlateStyle.SetPressed(PressedBrush);
		PlateStyle.SetDisabled(PlateBrush);
		CloseButton->SetStyle(PlateStyle);
	}
	else if (EnterRoomButton != nullptr)
	{
		// 판 텍스처가 없으면 ENTER와 같은 재질로 폴백한다.
		CloseButton->SetStyle(EnterRoomButton->GetStyle());
	}

	CloseButtonText = Cast<UTextBlock>(
		WidgetTree->FindWidget(TEXT("RuntimeCloseButtonText")));
	if (CloseButtonText == nullptr)
	{
		CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("RuntimeCloseButtonText"));
	}
	if (CloseButtonText != nullptr && CloseButtonText->GetParent() == nullptr)
	{
		CloseButtonText->SetText(mCloseText);
		CloseButtonText->SetJustification(ETextJustify::Center);
		if (EnterButtonText != nullptr)
		{
			CloseButtonText->SetFont(EnterButtonText->GetFont());
			CloseButtonText->SetColorAndOpacity(
				EnterButtonText->GetColorAndOpacity());
		}
		CloseButton->AddChild(CloseButtonText);
	}

	if (CloseButton->GetParent() == nullptr)
	{
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CloseButton);
		if (CanvasSlot != nullptr)
		{
			// 오른쪽 아래 구석(범례는 왼쪽 아래). 크기/여백은 화면에 맞춰
			// UpdateCloseButtonLayout()이 매 갱신마다 다시 잡는다.
			CanvasSlot->SetAnchors(FAnchors(1.f, 1.f));
			CanvasSlot->SetAlignment(FVector2D(1.f, 1.f));
			CanvasSlot->SetZOrder(500);
		}
	}
}

void UFrontendMapWidget::UpdateCloseButtonLayout() const
{
	if (CloseButton == nullptr)
	{
		return;
	}
	UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(CloseButton->Slot);
	if (ButtonSlot == nullptr)
	{
		return;
	}

	const FVector2D ScreenSize = GetCachedGeometry().GetLocalSize();
	if (ScreenSize.X <= 64.f || ScreenSize.Y <= 64.f)
	{
		return;   // 아직 레이아웃 전. 다음 갱신에서 다시 잡는다.
	}

	// 판 그림 비율(2.5:1)을 지켜야 모서리 리벳이 찌그러지지 않는다.
	const float ButtonWidth = FMath::Max(MapCloseButtonMinWidthPx,
		ScreenSize.X * MapCloseButtonWidthFrac);
	const float ButtonHeight = ButtonWidth / MapCloseButtonAspect;
	const float Margin = ScreenSize.X * MapLegendEdgeMarginFrac;

	ButtonSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
	ButtonSlot->SetAlignment(FVector2D(1.f, 1.f));
	ButtonSlot->SetAutoSize(false);
	ButtonSlot->SetPosition(FVector2D(-Margin, -Margin));
	ButtonSlot->SetSize(FVector2D(ButtonWidth, ButtonHeight));
	ApplyMapButtonFontSize(CloseButtonText, ButtonHeight);
}

void UFrontendMapWidget::ApplyCurrentMapArt() const
{
	if (Map_ParchmentBody != nullptr && mMapParchmentTexture != nullptr)
	{
		FSlateBrush Brush = Map_ParchmentBody->GetBrush();
		Brush.SetResourceObject(mMapParchmentTexture);
		/*
		 * 지도 본문은 평평한 통짜 그림이다. 눕힌 원근은 리테이너 머티리얼
		 * (M_MapPerspective)이 화면에서 걸므로 이미지/브러시에는 원근이 없어야
		 * 한다. 9-slice 를 쓰면 왜곡 후 프레임 경계가 찢어지므로 금지.
		 */
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Margin = FMargin(0.f);
		Map_ParchmentBody->SetBrush(Brush);
		Map_ParchmentBody->SetColorAndOpacity(FLinearColor::White);
		Map_ParchmentBody->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 지도 팝업 뒤 배경. 크기/위치(cover-fit)는 ConfigureMapGraphLayout이 잡는다.
	if (Map_Scrim != nullptr && mMapPopupBackgroundTexture != nullptr)
	{
		FSlateBrush ScrimBrush = Map_Scrim->GetBrush();
		ScrimBrush.SetResourceObject(mMapPopupBackgroundTexture);
		ScrimBrush.DrawAs = ESlateBrushDrawType::Image;
		ScrimBrush.Margin = FMargin(0.f);
		ScrimBrush.TintColor = FSlateColor(FLinearColor::White);
		Map_Scrim->SetBrush(ScrimBrush);
		Map_Scrim->SetColorAndOpacity(FLinearColor::White);
		Map_Scrim->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// B안 지도 자체에 상·하단 흑단/황동 프레임이 포함되어 있어 옛 청동
	// 두루마리 로드를 계속 숨긴다. 둘을 같이 켜면 폰에서 상하 테두리가 겹친다.
	if (Map_ScrollRodTop != nullptr)
	{
		Map_ScrollRodTop->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Map_ScrollRodBottom != nullptr)
	{
		Map_ScrollRodBottom->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFrontendMapWidget::ApplyLegendIconsV2() const
{
	UWidget* LegendRoot = Map_LegendGroup != nullptr
		? Map_LegendGroup.Get()
		: MapLegendList.Get();
	if (LegendRoot == nullptr || mMapIconV2ByKeyword.Num() == 0)
	{
		return;
	}

	TArray<UWidget*> Pending;
	Pending.Add(LegendRoot);
	while (Pending.Num() > 0)
	{
		UWidget* Current = Pending.Pop();
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Current))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				Pending.Add(Panel->GetChildAt(Index));
			}
		}
		UImage* IconImage = Cast<UImage>(Current);
		if (IconImage == nullptr)
		{
			continue;
		}
		UObject* Resource = IconImage->GetBrush().GetResourceObject();
		if (Resource == nullptr)
		{
			continue;
		}
		const FString ResourcePath = Resource->GetPathName().ToLower();
		if (ResourcePath.Contains(TEXT("_v2")))
		{
			continue;   // 이미 교체됨
		}
		for (const TPair<FString, TObjectPtr<UTexture2D>>& Entry : mMapIconV2ByKeyword)
		{
			if (Entry.Value != nullptr && ResourcePath.Contains(Entry.Key))
			{
				IconImage->SetBrushFromTexture(Entry.Value.Get());
				break;
			}
		}
	}
}

void UFrontendMapWidget::EnsureParchmentVeil()
{
	if (mMapParchmentVeil != nullptr || MapGraphCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	UImage* Veil = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("Map_ParchmentVeil"));
	if (Veil == nullptr)
	{
		return;
	}

	FSlateBrush VeilBrush;
	VeilBrush.DrawAs = ESlateBrushDrawType::Image;
	VeilBrush.TintColor = FSlateColor(FLinearColor::White);
	Veil->SetBrush(VeilBrush);
	Veil->SetColorAndOpacity(mMapParchmentVeilColor);
	Veil->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UCanvasPanelSlot* VeilSlot = MapGraphCanvas->AddChildToCanvas(Veil))
	{
		VeilSlot->SetAutoSize(false);
		// 양피지(-200)보다 위, 연결선(0)/노드(1)보다 아래.
		VeilSlot->SetZOrder(-150);
	}
	mMapParchmentVeil = Veil;
}

void UFrontendMapWidget::EnsureLegendPlate()
{
	if (mMapLegendTexture == nullptr || Map_LegendGroup == nullptr
		|| WidgetTree == nullptr)
	{
		return;
	}

	UCanvasPanel* ParentCanvas = Cast<UCanvasPanel>(Map_LegendGroup->GetParent());
	UCanvasPanelSlot* GroupSlot = Cast<UCanvasPanelSlot>(Map_LegendGroup->Slot);
	if (ParentCanvas == nullptr || GroupSlot == nullptr)
	{
		return;
	}

	if (mMapLegendPlate == nullptr)
	{
		mMapLegendPlate = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("Map_LegendPlate"));
		if (mMapLegendPlate == nullptr)
		{
			return;
		}

		FSlateBrush PlateBrush;
		PlateBrush.DrawAs = ESlateBrushDrawType::Image;
		PlateBrush.SetResourceObject(mMapLegendTexture);
		mMapLegendPlate->SetBrush(PlateBrush);
		// 보임/숨김은 여닫기 단추 상태(ApplyLegendShownState)가 정한다.

		if (UCanvasPanelSlot* PlateSlot = ParentCanvas->AddChildToCanvas(mMapLegendPlate))
		{
			// 시안 범례가 놓였던 자리(앵커/정렬/위치)를 그대로 물려받는다. 크기는
			// 화면에 맞춰 UpdateLegendPlateLayout()이 매 갱신마다 다시 잡는다.
			PlateSlot->SetAnchors(GroupSlot->GetAnchors());
			PlateSlot->SetAlignment(GroupSlot->GetAlignment());
			PlateSlot->SetPosition(GroupSlot->GetPosition());
			PlateSlot->SetAutoSize(false);
			PlateSlot->SetZOrder(GroupSlot->GetZOrder());
		}
	}

	// 시안 범례 행(아이콘+글자)은 새 그림에 이미 그려져 있어 켜두면 겹친다.
	Map_LegendGroup->SetVisibility(ESlateVisibility::Collapsed);
}

void UFrontendMapWidget::EnsureLegendToggleButton()
{
	if (mMapLegendToggleButton != nullptr || WidgetTree == nullptr
		|| mMapLegendPlate == nullptr)
	{
		return;
	}
	UCanvasPanel* ParentCanvas = Cast<UCanvasPanel>(mMapLegendPlate->GetParent());
	if (ParentCanvas == nullptr)
	{
		return;
	}

	mMapLegendToggleButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), TEXT("RuntimeLegendToggleButton"));
	if (mMapLegendToggleButton == nullptr)
	{
		return;
	}
	if (CloseButton != nullptr)
	{
		mMapLegendToggleButton->SetStyle(CloseButton->GetStyle());
	}

	mMapLegendToggleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("RuntimeLegendToggleText"));
	if (mMapLegendToggleText != nullptr)
	{
		mMapLegendToggleText->SetJustification(ETextJustify::Center);
		if (CloseButtonText != nullptr)
		{
			mMapLegendToggleText->SetFont(CloseButtonText->GetFont());
			mMapLegendToggleText->SetColorAndOpacity(
				CloseButtonText->GetColorAndOpacity());
		}
		mMapLegendToggleButton->AddChild(mMapLegendToggleText);
	}

	if (UCanvasPanelSlot* ToggleSlot = ParentCanvas->AddChildToCanvas(mMapLegendToggleButton))
	{
		ToggleSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		ToggleSlot->SetAlignment(FVector2D(0.f, 1.f));
		ToggleSlot->SetAutoSize(false);
		ToggleSlot->SetZOrder(500);
	}
	mMapLegendToggleButton->OnClicked.AddUniqueDynamic(
		this, &UFrontendMapWidget::HandleLegendToggleClicked);
}

void UFrontendMapWidget::HandleLegendToggleClicked()
{
	mMapLegendShown = !mMapLegendShown;
	ApplyLegendShownState();
}

void UFrontendMapWidget::ApplyLegendShownState() const
{
	if (mMapLegendPlate != nullptr)
	{
		mMapLegendPlate->SetVisibility(mMapLegendShown
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (mMapLegendToggleText != nullptr)
	{
		mMapLegendToggleText->SetText(FrontendMapText(TEXT("LegendText")));
	}
}

void UFrontendMapWidget::ApplyMapButtonFontSize(UTextBlock* ButtonText, float ButtonHeight) const
{
	if (ButtonText == nullptr || ButtonHeight <= 1.f)
	{
		return;
	}
	/*
	 * 단추 크기는 화면 폭을 따라가는데 글자는 시안 고정 크기라 큰 화면에서
	 * 글자만 작게 남는다. 판 높이에 비례시켜 어느 화면에서도 같은 비율로 읽히게 한다.
	 */
	FSlateFontInfo Font = ButtonText->GetFont();
	Font.Size = FMath::Max(8.f, ButtonHeight * 0.34f);
	ButtonText->SetFont(Font);
}

void UFrontendMapWidget::UpdateLegendPlateLayout() const
{
	if (mMapLegendPlate == nullptr || mMapLegendTexture == nullptr)
	{
		return;
	}
	UCanvasPanelSlot* PlateSlot = Cast<UCanvasPanelSlot>(mMapLegendPlate->Slot);
	if (PlateSlot == nullptr)
	{
		return;
	}

	FVector2D ScreenSize = GetCachedGeometry().GetLocalSize();
	if (ScreenSize.X <= 64.f || ScreenSize.Y <= 64.f)
	{
		return;   // 아직 레이아웃 전. 다음 갱신에서 다시 잡는다.
	}

	const float TextureAspect = mMapLegendTexture->GetSizeY() > 0
		? StaticCast<float>(mMapLegendTexture->GetSizeX())
			/ StaticCast<float>(mMapLegendTexture->GetSizeY())
		: 0.667f;

	float PlateWidth = ScreenSize.X * MapLegendWidthFrac;
	float PlateHeight = PlateWidth / FMath::Max(0.1f, TextureAspect);

	// 짧은 화면에서 범례가 세로를 다 덮지 않게 높이를 먼저 제한하고 폭을 되돌린다.
	const float MaxHeight = ScreenSize.Y * MapLegendMaxHeightFrac;
	if (PlateHeight > MaxHeight)
	{
		PlateHeight = MaxHeight;
		PlateWidth = PlateHeight * TextureAspect;
	}
	/*
	 * 왼쪽 아래 구석의 여닫기 단추 바로 위에 붙인다. 정렬을 (0,1)로 두면 판이
	 * 커져도 화면 안쪽(위/오른쪽)으로만 자라 잘리지 않는다.
	 * 오른쪽 아래는 BACK 단추 자리다.
	 */
	const float Margin = ScreenSize.X * MapLegendEdgeMarginFrac;
	const float ToggleWidth = FMath::Max(MapCloseButtonMinWidthPx,
		ScreenSize.X * MapCloseButtonWidthFrac);
	const float ToggleHeight = ToggleWidth / MapCloseButtonAspect;

	if (mMapLegendToggleButton != nullptr)
	{
		if (UCanvasPanelSlot* ToggleSlot = Cast<UCanvasPanelSlot>(mMapLegendToggleButton->Slot))
		{
			ToggleSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
			ToggleSlot->SetAlignment(FVector2D(0.f, 1.f));
			ToggleSlot->SetAutoSize(false);
			ToggleSlot->SetPosition(FVector2D(Margin, -Margin));
			ToggleSlot->SetSize(FVector2D(ToggleWidth, ToggleHeight));
		}
		ApplyMapButtonFontSize(mMapLegendToggleText, ToggleHeight);
	}

	// 판이 단추를 덮지 않도록 단추 높이만큼 위로 올린다.
	const float PlateBottomOffset = mMapLegendToggleButton != nullptr
		? Margin + ToggleHeight + Margin * 0.5f
		: Margin;
	PlateSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
	PlateSlot->SetAlignment(FVector2D(0.f, 1.f));
	PlateSlot->SetPosition(FVector2D(Margin, -PlateBottomOffset));
	PlateSlot->SetSize(FVector2D(PlateWidth, PlateHeight));
}

void UFrontendMapWidget::InstallMapPerspectiveRetainer()
{
	if (!mUseMapPerspective || MapScrollBox == nullptr
		|| mMapPerspectiveMaterial == nullptr || WidgetTree == nullptr)
	{
		return;
	}
	// 위젯 재사용(닫았다 다시 열기)으로 NativeConstruct가 다시 와도 한 번만 감싼다.
	if (mMapPerspectiveRetainer != nullptr
		&& MapScrollBox->GetParent() == mMapPerspectiveRetainer)
	{
		return;
	}

	UPanelWidget* ParentPanel = MapScrollBox->GetParent();
	if (ParentPanel == nullptr)
	{
		return;
	}

	URetainerBox* Retainer = WidgetTree->ConstructWidget<URetainerBox>(
		URetainerBox::StaticClass(), TEXT("MapPerspectiveRetainer"));
	if (Retainer == nullptr)
	{
		return;
	}

	// ReplaceChild는 WITH_EDITOR 전용 API다. 패키지 빌드에서도 같은 위치와
	// 슬롯 레이아웃을 보존하도록 런타임 지원 Remove/Insert 경로를 사용한다.
	const int32 ChildIndex = ParentPanel->GetChildIndex(MapScrollBox);
	UPanelSlot* SlotTemplate = MapScrollBox->Slot;
	if (ChildIndex == INDEX_NONE || SlotTemplate == nullptr
		|| !ParentPanel->RemoveChildAt(ChildIndex))
	{
		return;
	}
	if (ParentPanel->InsertChildAt(ChildIndex, Retainer, SlotTemplate) == nullptr)
	{
		// 삽입 실패 시 원래 위젯을 복구해 빈 지도 컨테이너가 남지 않게 한다.
		ParentPanel->InsertChildAt(ChildIndex, MapScrollBox, SlotTemplate);
		return;
	}
	Retainer->AddChild(MapScrollBox);

	Retainer->SetRetainRendering(true);
	Retainer->SetTextureParameter(TEXT("Texture"));
	Retainer->SetEffectMaterial(mMapPerspectiveMaterial);
	if (UMaterialInstanceDynamic* EffectMID = Retainer->GetEffectMaterial())
	{
		EffectMID->SetScalarParameterValue(TEXT("TopWidth"), MapPerspectiveTopWidth);
	}

	mMapPerspectiveRetainer = Retainer;
}

FVector2D UFrontendMapWidget::InverseMapPerspectiveUV(const FVector2D& ScreenUV)
{
	/*
	 * M_MapPerspective 의 HLSL과 같은 사영 변환(대칭 사다리꼴 호모그래피)의
	 * 역이다. 화면 v -> 콘텐츠 t 는 원근 단축 때문에 비선형이다.
	 * 머티리얼과 이 함수가 어긋나면 보이는 노드와 눌리는 노드가 어긋난다.
	 */
	const float W = FMath::Clamp(MapPerspectiveTopWidth, 0.05f, 1.f);
	const float V = FMath::Clamp(ScreenUV.Y, 0.f, 1.f);
	const float T = V / (W + (1.f - W) * V);
	const float D = (W - 1.f) * T + 1.f;
	const float S = (ScreenUV.X * D - (0.5f - 0.5f * W) * (1.f - T)) / W;
	return FVector2D(S, T);
}

bool UFrontendMapWidget::TryHandleMapPerspectiveTap(const FVector2D& ScreenPosition)
{
	if (mMapPerspectiveRetainer == nullptr || MapScrollBox == nullptr
		|| mMapTapTargets.Num() == 0)
	{
		return false;
	}
	// 노드 버튼 대신 지도 레벨에서 탭을 받으므로, 선택 허용 여부도 여기서 막는다.
	if (!IsRoomSelectionEnabled() || mEnterRequested)
	{
		return false;
	}

	const FGeometry& RetainerGeometry = mMapPerspectiveRetainer->GetCachedGeometry();
	const FVector2D LocalSize = RetainerGeometry.GetLocalSize();
	if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D Local = RetainerGeometry.AbsoluteToLocal(ScreenPosition);
	const FVector2D ContentUV = InverseMapPerspectiveUV(Local / LocalSize);
	if (ContentUV.X < 0.f || ContentUV.X > 1.f)
	{
		return false;   // 사다리꼴 밖(책상 영역) 탭
	}

	const FVector2D GraphSize = GetMapGraphContentSize();
	const FVector2D ContentPoint(
		ContentUV.X * GraphSize.X,
		ContentUV.Y * LocalSize.Y + MapScrollBox->GetScrollOffset());

	const FVector2D NodeSize = GetMapNodeSize();
	const float TapRadius = FMath::Max(NodeSize.X, NodeSize.Y) * 0.75f;
	float BestDistanceSquared = TapRadius * TapRadius;
	const FMapTapTarget* BestTarget = nullptr;
	for (const FMapTapTarget& Target : mMapTapTargets)
	{
		if (!Target.mSelectable)
		{
			continue;
		}
		const float DistanceSquared = (Target.mCenter - ContentPoint).SizeSquared();
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = &Target;
		}
	}
	if (BestTarget == nullptr)
	{
		return false;
	}

	HandleMapRoomClicked(BestTarget->mCoord.X, BestTarget->mCoord.Y);
	return true;
}

/**
 * @brief 버튼/노드 이벤트 연결과 동적 생성한 그래프 풀을 정리한다.
 */
void UFrontendMapWidget::NativeDestruct()
{
	mMapDragScrolling = false;
	OnNativeVisibilityChanged.RemoveAll(this);
	if (mRunOptionsRailWidget != nullptr)
	{
		mRunOptionsRailWidget->RemoveFromParent();
		mRunOptionsRailWidget = nullptr;
	}
	UnbindEvents();
	for (FFrontendMapNodePoolEntry& NodeEntry : mMapNodePool)
	{
		if (NodeEntry.mNodeWidget != nullptr)
		{
			NodeEntry.mNodeWidget->OnMapNodeClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleMapNodeClicked);
		}
	}
	/*
	 * 풀을 비우지 않는다.
	 *
	 * 노드/선 위젯은 MapGraphCanvas의 자식으로 남아 있는데, 풀 배열만 비우면
	 * 다음 NativeConstruct에서 같은 수만큼 새로 만들어 캔버스에 또 얹는다.
	 * 옛 위젯은 캔버스에 그대로 보이므로 지도가 두 벌로 겹쳐 보인다
	 * (실측: 풀 25개인데 화면에 보이는 노드 50개).
	 * 델리게이트만 끊고 위젯은 남겨 두었다가 Acquire에서 다시 묶어 재사용한다.
	 */
	Super::NativeDestruct();
}

void UFrontendMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	/*
	 * 뷰포트가 바뀌면(창 리사이즈/회전) 그래프 폭·높이와 노드 배치가 낡은 값으로 남는다.
	 * 몇 프레임 안정된 뒤 전체 리프레시로 다시 깐다(라이브 리사이즈 재배치).
	 */
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (UGameViewportClient* ViewportClient = GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr)
	{
		ViewportClient->GetViewportSize(OUT ViewportSize);
	}
	if (!ViewportSize.Equals(mLastViewportSize, 0.5f))
	{
		mLastViewportSize = ViewportSize;
		mResizeRefreshCountdown = 3;
	}
	else if (mResizeRefreshCountdown > 0 && --mResizeRefreshCountdown == 0)
	{
		RefreshMap();
	}
}

FReply UFrontendMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MapScrollBox != nullptr)
	{
		const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		if (MapScrollBox->GetCachedGeometry().IsUnderLocation(ScreenPosition))
		{
			mMapDragScrolling = true;
			mMapDragLastScreenPosition = ScreenPosition;
			mMapDragStartScreenPosition = ScreenPosition;
			MapScrollBox->EndInertialScrolling();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UFrontendMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && mMapDragScrolling)
	{
		mMapDragScrolling = false;

		/*
		 * 원근 리테이너가 켜지면 노드 버튼은 히트테스트를 받지 않는다(그리는
		 * 위치와 버튼 위치가 어긋나므로). 이동량이 작은 눌림은 탭으로 보고
		 * 좌표를 역변환해 노드 클릭으로 처리한다.
		 */
		const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		const float TapSlopPx = 12.f;
		if ((ScreenPosition - mMapDragStartScreenPosition).SizeSquared()
			<= TapSlopPx * TapSlopPx)
		{
			TryHandleMapPerspectiveTap(ScreenPosition);
		}
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UFrontendMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (mMapDragScrolling && MapScrollBox != nullptr)
	{
		if (!InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			mMapDragScrolling = false;
			return FReply::Handled().ReleaseMouseCapture();
		}

		const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		const float DeltaY = ScreenPosition.Y - mMapDragLastScreenPosition.Y;
		if (!FMath::IsNearlyZero(DeltaY))
		{
			MapScrollBox->SetScrollOffset(FMath::Max(0.0f, MapScrollBox->GetScrollOffset() - DeltaY));
			mMapDragLastScreenPosition = ScreenPosition;
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

/**
 * @brief 지도 화면에서 사용하는 기본 문구 캐시를 갱신한다.
 */
void UFrontendMapWidget::RefreshLocalizedTextCache()
{
	mMapText = FrontendMapText(TEXT("MapText"));
	mCloseText = FrontendMapText(TEXT("CloseText"));
	mEnterText = FrontendMapText(TEXT("EnterText"));
	mLoadingStatusText = FrontendMapText(TEXT("LoadingStatusText"));
	mMapReadyStatusText = FText::GetEmpty();
	mMapUnavailableStatusText = FrontendMapText(TEXT("MapUnavailableStatusText"));
}

/**
 * @brief 현재 지도에서 방 선택 입력을 허용할지 지정한다.
 *
 * @details
 * 같은 월드맵 위젯을 조회용과 전투 승리 후 선택용으로 공유하기 위해 입력 가능 여부를 외부에서 제어한다.
 */
void UFrontendMapWidget::SetRoomSelectionEnabled(bool bEnabled)
{
	mRoomSelectionEnabled = bEnabled;
}

/**
 * @brief 현재 지도 입력 허용 상태를 반환한다.
 */
bool UFrontendMapWidget::IsRoomSelectionEnabled() const
{
	return mRoomSelectionEnabled;
}

/**
 * @brief 지도 기본 상태 문구 대신 외부 흐름의 안내 문구를 표시한다.
 */
void UFrontendMapWidget::SetMapStatusOverride(const FText& InText)
{
	mStatusOverrideText = InText;
	SetMapStatusText(mStatusOverrideText);
}

/**
 * @brief 외부 상태 문구를 해제하고 현재 캐시된 기본 상태 문구를 즉시 표시한다.
 */
void UFrontendMapWidget::ClearMapStatusOverride()
{
	mStatusOverrideText = FText::GetEmpty();
	SetMapStatusText(mMapReadyStatusText);
}

void UFrontendMapWidget::ValidateDesignerBindings() const
{
	/*
	 * 지도 위젯은 WBP_FrontendMap 안에 Graph Canvas와 ScrollBox, 버튼, 노드/선 클래스가 모두 연결되어야 동작한다.
	 * 여기서 즉시 크래시하지 않고 경고만 남기는 이유는 UI 담당자가 WBP 배치를 고치는 중에도 에디터에서 화면을 열어 확인할 수 있게 하기 위해서다.
	 */
	if (MapGraphCanvas == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapGraphCanvas is not connected. WBP_FrontendMap must provide the graph canvas."));
	}
	if (MapGraphSize == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapGraphSize is not connected. WBP_FrontendMap should own the graph size box."));
	}
	if (MapScrollBox == nullptr && !IsLandscapeLayout())
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapScrollBox is not connected. WBP_FrontendMap should own the scroll box."));
	}
	if (CloseButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: CloseButton is not connected."));
	}
	if (EnterRoomButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: EnterRoomButton is not connected."));
	}
	if (MapLineWidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapLineWidgetClass is not set. Set WBP_FrontendMapLine in WBP_FrontendMap Class Defaults."));
	}
	if (MapNodeWidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: MapNodeWidgetClass is not set. Set WBP_FrontendMapNode in WBP_FrontendMap Class Defaults."));
	}
}

void UFrontendMapWidget::BindEvents()
{
	/*
	 * 버튼 클릭은 지도 내부 처리로 끝내지 않고, Close는 외부 요청 이벤트로,
	 * Enter는 RoomGameMode 전환 요청으로 연결한다.
	 */
	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UFrontendMapWidget::HandleCloseButtonClicked);
	}
	if (EnterRoomButton != nullptr)
	{
		EnterRoomButton->OnClicked.AddUniqueDynamic(this, &UFrontendMapWidget::HandleEnterRoomButtonClicked);
	}
}

void UFrontendMapWidget::UnbindEvents()
{
	/* NativeConstruct에서 붙인 버튼 델리게이트를 제거해 위젯 재생성 시 중복 호출을 막는다. */
	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleCloseButtonClicked);
	}
	if (EnterRoomButton != nullptr)
	{
		EnterRoomButton->OnClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleEnterRoomButtonClicked);
	}
}

FFrontendMapLinePoolEntry* UFrontendMapWidget::AcquireMapLineWidget(int32 LineIndex)
{
	/*
	 * 지도 연결선은 방 개수에 따라 매번 필요한 수가 달라진다.
	 * RefreshMap()마다 새 위젯을 전부 만들면 비용과 GC 부담이 커지므로, 필요한 인덱스까지 풀을 늘리고 재사용한다.
	 */
	if (MapGraphCanvas == nullptr || MapLineWidgetClass == nullptr || LineIndex < 0)
	{
		return nullptr;
	}

	while (mMapLinePool.Num() <= LineIndex)
	{
		FFrontendMapLinePoolEntry NewEntry;
		NewEntry.mLineWidget = CreateWidget<UFrontendMapLineWidget>(this, MapLineWidgetClass);
		if (NewEntry.mLineWidget != nullptr)
		{
			NewEntry.mLineWidget->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
			NewEntry.mLineWidget->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* LineSlot = MapGraphCanvas->AddChildToCanvas(NewEntry.mLineWidget))
			{
				LineSlot->SetAutoSize(false);
				LineSlot->SetZOrder(0);
			}
		}
		mMapLinePool.Add(MoveTemp(NewEntry));
	}

	FFrontendMapLinePoolEntry& Entry = mMapLinePool[LineIndex];
	if (Entry.mLineWidget != nullptr)
	{
		Entry.mLineWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return &Entry;
}

FFrontendMapNodePoolEntry* UFrontendMapWidget::AcquireMapNodeWidget(int32 NodeIndex)
{
	/*
	 * 지도 노드도 선과 동일하게 풀링한다.
	 * 새 노드를 만들 때만 OnMapNodeClicked를 연결하고, 이후 RefreshMap()에서는 데이터와 위치만 다시 적용한다.
	 */
	if (MapGraphCanvas == nullptr || MapNodeWidgetClass == nullptr || NodeIndex < 0)
	{
		return nullptr;
	}

	while (mMapNodePool.Num() <= NodeIndex)
	{
		FFrontendMapNodePoolEntry NewEntry;
		NewEntry.mNodeWidget = CreateWidget<UFrontendMapNodeWidget>(this, MapNodeWidgetClass);
		if (NewEntry.mNodeWidget != nullptr)
		{
			NewEntry.mNodeWidget->SetVisibility(ESlateVisibility::Collapsed);
			NewEntry.mNodeWidget->OnMapNodeClicked.AddUniqueDynamic(this, &UFrontendMapWidget::HandleMapNodeClicked);

			if (UCanvasPanelSlot* NodeSlot = MapGraphCanvas->AddChildToCanvas(NewEntry.mNodeWidget))
			{
				NodeSlot->SetAutoSize(false);
				NodeSlot->SetSize(GetMapNodeSize());
				NodeSlot->SetZOrder(1);
			}
		}
		mMapNodePool.Add(MoveTemp(NewEntry));
	}

	FFrontendMapNodePoolEntry& Entry = mMapNodePool[NodeIndex];
	if (Entry.mNodeWidget != nullptr)
	{
		// NativeDestruct에서 끊긴 델리게이트를 다시 묶는다(재사용 경로). 중복 등록은 없다.
		Entry.mNodeWidget->OnMapNodeClicked.AddUniqueDynamic(
			this, &UFrontendMapWidget::HandleMapNodeClicked);
		/*
		 * 원근 리테이너가 켜지면 노드가 그려지는 화면 위치와 버튼의 히트테스트
		 * 위치가 어긋난다(왜곡은 그리기에만 적용). 버튼 입력을 끄고 지도
		 * 레벨의 탭 역변환(TryHandleMapPerspectiveTap)이 클릭을 대신한다.
		 */
		Entry.mNodeWidget->SetVisibility(mMapPerspectiveRetainer != nullptr
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Visible);
	}
	return &Entry;
}

void UFrontendMapWidget::CollapseOrphanGraphWidgets() const
{
	/*
	 * 풀에 없는데 캔버스에 남아 있는 노드/선을 접는다.
	 *
	 * 풀을 유지하도록 고쳤으므로 이제 고아는 생기지 않지만, 이전 빌드에서 만든
	 * 위젯이 저장된 화면에 남아 있거나 다른 경로로 캔버스에 얹히면 지도가 두 벌로
	 * 보인다. 그리기 전에 한 번 훑어 내 풀 소속이 아닌 것은 숨긴다.
	 */
	if (MapGraphCanvas == nullptr)
	{
		return;
	}

	TSet<const UWidget*> PooledWidgets;
	PooledWidgets.Reserve(mMapNodePool.Num() + mMapLinePool.Num());
	for (const FFrontendMapNodePoolEntry& Entry : mMapNodePool)
	{
		PooledWidgets.Add(Entry.mNodeWidget);
	}
	for (const FFrontendMapLinePoolEntry& Entry : mMapLinePool)
	{
		PooledWidgets.Add(Entry.mLineWidget);
	}

	for (int32 Index = 0; Index < MapGraphCanvas->GetChildrenCount(); ++Index)
	{
		UWidget* Child = MapGraphCanvas->GetChildAt(Index);
		if (Child == nullptr || PooledWidgets.Contains(Child))
		{
			continue;
		}
		if (Child->IsA<UFrontendMapNodeWidget>() || Child->IsA<UFrontendMapLineWidget>())
		{
			Child->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UFrontendMapWidget::HideUnusedMapGraphWidgets(int32 UsedLineCount, int32 UsedNodeCount)
{
	/*
	 * 이전 지도 갱신에서 더 많은 노드/선이 필요했다면 풀 안에 남은 위젯이 있을 수 있다.
	 * 이번 갱신에서 사용하지 않는 나머지는 제거하지 않고 숨겨 다음 갱신 때 재사용한다.
	 */
	for (int32 LineIndex = UsedLineCount; LineIndex < mMapLinePool.Num(); ++LineIndex)
	{
		if (UFrontendMapLineWidget* LineWidget = mMapLinePool[LineIndex].mLineWidget)
		{
			LineWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	for (int32 NodeIndex = UsedNodeCount; NodeIndex < mMapNodePool.Num(); ++NodeIndex)
	{
		if (UFrontendMapNodeWidget* NodeWidget = mMapNodePool[NodeIndex].mNodeWidget)
		{
			NodeWidget->SetNodeEnabled(false);
			NodeWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

/**
 * @brief 현재 RunPersistData 기반 지도 노드/선을 다시 그리고 버튼 상태를 갱신한다.
 *
 * @details
 * 같은 월드맵이 조회용과 다음 방 선택용으로 쓰이므로, mRoomSelectionEnabled와 mEnterRequested를 함께 보고
 * 노드/입장 버튼의 입력 가능 여부를 결정한다. 상태 문구는 전투 승리 같은 외부 흐름의 오버라이드가 있으면 그것을 우선한다.
 *
 * 왜 매번 View DTO를 다시 가져오는가:
 * 지도 노드를 클릭하면 선택 상태가 GameMode의 런 데이터에 반영되고, 그 결과로 Ready/Selected/Locked 표시가 바뀐다.
 * UI가 자체 캐시만 믿지 않고 다시 그리면 실제 런 상태와 화면 표시가 어긋나지 않는다.
 */
bool UFrontendMapWidget::RefreshMap()
{
	RefreshLocalizedTextCache();
	HideUnusedMapTextSurfaces();
	mEnterRequested = false;

	if (MapGraphCanvas == nullptr)
	{
		return false;
	}

	TArray<FMapRoomView> Rooms;
	bool bHasRooms = false;
	bool bShouldScrollToStart = false;
	if (mUseDebugPreviewRooms)
	{
		Rooms = mDebugPreviewRooms;
		bHasRooms = Rooms.IsEmpty() == false;
		bShouldScrollToStart = mDebugPreviewAtStageStart;
	}
	else if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		bHasRooms = RoomGameMode->GetMapRoomViews(Rooms);

		FRunControlView RunControlView;
		bShouldScrollToStart = RoomGameMode->GetRunControlView(OUT RunControlView) && RunControlView.mIsAtStageStart;
	}
	else
	{
		const bool bGameWorld = GetWorld() != nullptr && GetWorld()->IsGameWorld();
		if (bGameWorld)
		{
			UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: RoomGameMode is not available. WorldMap data must be provided by RoomGameMode."));
		}
		else
		{
			UE_LOG(LogRD, Verbose, TEXT("FrontendMapWidget: editor preview has no RoomGameMode."));
		}
	}

	// 진단(0807): 빈 지도가 보고됐는데 경고 로그가 하나도 없었다. 성공 경로도
	// 수를 남겨야 "데이터가 없었는지, 그리기가 실패했는지" 를 가를 수 있다.
	UE_LOG(LogRD, Log, TEXT("[지도] RefreshMap: 방 %d개 (데이터 %s)"),
		Rooms.Num(), bHasRooms ? TEXT("있음") : TEXT("없음"));

	// 그래프 높이는 행 수 x 행 간격이므로, 데이터를 읽은 뒤에 레이아웃을 확정한다.
	int32 MaxRowIndex = 0;
	for (const FMapRoomView& Room : Rooms)
	{
		MaxRowIndex = FMath::Max(MaxRowIndex, Room.mRow);
	}
	mCachedRowCount = bHasRooms ? MaxRowIndex + 1 : 0;

	// 노드 크기는 열 간격에서 파생하므로 레이아웃보다 먼저 확정한다
	// (그래프 높이 계산이 노드 크기를 쓴다).
	ResolveNodeMetrics(Rooms);

	ConfigureMapGraphLayout();

	if (!bHasRooms)
	{
		HideUnusedMapGraphWidgets(0, 0);
		SetMapStatusText(mMapUnavailableStatusText);
		SetMapPreviewText(FrontendMapText(TEXT("NoRoomSelected")), mMapUnavailableStatusText, FText::GetEmpty(), MutedColor);
		SetEnterButtonVisible(false);
		if (EnterRoomButton != nullptr)
		{
			EnterRoomButton->SetIsEnabled(false);
		}
		return false;
	}

	bool bHasSelectedRoom = false;
	FText SelectedRoomTitle;
	FText SelectedRoomDescription;
	FText SelectedRoomState;
	FSlateColor SelectedRoomStateColor = MutedColor;
	UWidget* FocusMapNodeWidget = nullptr;
	UWidget* CurrentRoomNodeWidget = nullptr;   // 진입 가능 방이 없을 때(전투 승리 직후 등) 스크롤 폴백 대상.
	const FVector2D GraphSize = GetMapGraphContentSize();

	TMap<FIntPoint, FVector2D> NodeCenters;
	mMapTapTargets.Reset();
	for (const FMapRoomView& Room : Rooms)
	{
		const FVector2D Center = GetMapRoomNodeCenter(Rooms, Room, GraphSize);
		NodeCenters.Add(FIntPoint(Room.mRow, Room.mColumn), Center);
		// 원근 탭 역변환용 캐시. 선택 가능 여부는 최종적으로 GameMode가 다시 검증한다.
		FMapTapTarget TapTarget;
		TapTarget.mCoord = FIntPoint(Room.mRow, Room.mColumn);
		TapTarget.mCenter = Center;
		TapTarget.mSelectable = Room.mSelectable;
		mMapTapTargets.Add(TapTarget);
	}

	// 현재 위치 = 방문(Cleared) 경로의 마지막 행. 선택된 경로에서는 행마다 방문 방이 하나뿐이다.
	FIntPoint CurrentCoord(INDEX_NONE, INDEX_NONE);
	FIntPoint SelectedCoord(INDEX_NONE, INDEX_NONE);
	for (const FMapRoomView& Room : Rooms)
	{
		if (Room.mState == EMapRoomState::Cleared && (CurrentCoord.X == INDEX_NONE || Room.mRow > CurrentCoord.X))
		{
			CurrentCoord = FIntPoint(Room.mRow, Room.mColumn);
		}
		if (Room.mSelected)
		{
			SelectedCoord = FIntPoint(Room.mRow, Room.mColumn);
		}
	}

	int32 UsedLineCount = 0;
	for (const FMapRoomView& Room : Rooms)
	{
		const FVector2D* FromCenter = NodeCenters.Find(FIntPoint(Room.mRow, Room.mColumn));
		if (FromCenter == nullptr)
		{
			continue;
		}

		for (int32 NextColumn : Room.mNextRoomColumns)
		{
			const FMapRoomView* NextRoom = FindMapRoom(Rooms, Room.mRow + 1, NextColumn);
			const FVector2D* ToCenter = NodeCenters.Find(FIntPoint(Room.mRow + 1, NextColumn));
			if (NextRoom == nullptr || ToCenter == nullptr)
			{
				continue;
			}

			const FVector2D Delta = *ToCenter - *FromCenter;
			const float Length = Delta.Size();
			if (Length <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			FFrontendMapLinePoolEntry* LineEntry = AcquireMapLineWidget(UsedLineCount++);
			if (LineEntry == nullptr || LineEntry->mLineWidget == nullptr)
			{
				continue;
			}

			UFrontendMapLineWidget* ConnectionLine = LineEntry->mLineWidget;
			/*
			 * 실제로 지나온 길은 양쪽 방이 모두 Cleared일 때만 밝힌다.
			 * 현재 방에서 다음 후보로 뻗는 길은 승리 후 방 선택 모드에서만 연다.
			 * 따라서 전투 중 MAP 조회에서는 Ready 노드가 보여도 다음 길은 잠긴 점선으로 남는다.
			 */
			const bool bIsTraversedPath =
				Room.mState == EMapRoomState::Cleared
				&& NextRoom->mState == EMapRoomState::Cleared;
			const bool bIsCurrentRoom =
				FIntPoint(Room.mRow, Room.mColumn) == CurrentCoord;
			const bool bIsNextRoomCandidate =
				NextRoom->mState == EMapRoomState::Ready
				|| NextRoom->mState == EMapRoomState::Selected;
			const bool bIsOpenNextPath =
				mRoomSelectionEnabled && bIsCurrentRoom && bIsNextRoomCandidate;
			ConnectionLine->SetLineStyle(
				bIsTraversedPath || bIsOpenNextPath,
				bIsTraversedPath);

			FWidgetTransform LineTransform;
			LineTransform.Angle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			ConnectionLine->SetRenderTransform(LineTransform);

			// 시안 두께(14)는 1920 기준이라 폰에서 실처럼 얇다. 노드가 커지면
			// 선도 같이 굵어지도록 노드 지름에 비례시키고, 시안 값을 하한으로 둔다.
			const float LineThickness = FMath::Max(ConnectionLine->GetLineThickness(),
				GetMapNodeSize().X * MapLineThicknessNodeRatio);
			if (UCanvasPanelSlot* LineSlot = Cast<UCanvasPanelSlot>(ConnectionLine->Slot))
			{
				LineSlot->SetSize(FVector2D(Length, LineThickness));
				LineSlot->SetPosition(*FromCenter - FVector2D(0.f, LineThickness * 0.5f));
				LineSlot->SetZOrder(0);
			}
		}
	}

	int32 UsedNodeCount = 0;
	for (const FMapRoomView& Room : Rooms)
	{
		const FText StateText = GetMapRoomStateText(Room);
		const FSlateColor StateColor = Room.mState == EMapRoomState::Locked
			? LockedColor
			: (Room.mState == EMapRoomState::Selected ? TitleColor : ReadyColor);

		const bool bShouldFocusRoom = Room.mCanEnter || Room.mSelected;
		if (bShouldFocusRoom)
		{
			bHasSelectedRoom = true;
			SelectedRoomTitle = Room.mTitle;
			SelectedRoomDescription = Room.mDescription;
			SelectedRoomState = StateText;
			SelectedRoomStateColor = StateColor;
		}

		FFrontendMapNodePoolEntry* NodeEntry = AcquireMapNodeWidget(UsedNodeCount++);
		if (NodeEntry == nullptr || NodeEntry->mNodeWidget == nullptr)
		{
			continue;
		}

		UFrontendMapNodeWidget* NodeWidget = NodeEntry->mNodeWidget;
		NodeWidget->SetNodeEnabled(Room.mSelectable && IsFrontendMapNavigationEnabled() && !mEnterRequested);
		NodeWidget->SetNodeVisual(
			Room.mRow,
			Room.mColumn,
			GetMapRoomDebugNodeLabel(Room),
			FText::GetEmpty(),
			GetMapRoomPanelColor(Room),
			GetMapRoomTypeColor(Room.mType),
			GetMapRoomTextColor(Room),
			FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.f)),
			Room.mType,
			Room.mState,
			FIntPoint(Room.mRow, Room.mColumn) == CurrentCoord);
		if (bShouldFocusRoom)
		{
			FocusMapNodeWidget = NodeWidget;
		}
		if (FIntPoint(Room.mRow, Room.mColumn) == CurrentCoord)
		{
			CurrentRoomNodeWidget = NodeWidget;
		}

		const FVector2D NodeSize = GetMapNodeSize();
		const FVector2D Center = NodeCenters.FindChecked(FIntPoint(Room.mRow, Room.mColumn));
		if (UCanvasPanelSlot* NodeSlot = Cast<UCanvasPanelSlot>(NodeWidget->Slot))
		{
			NodeSlot->SetSize(NodeSize);
			NodeSlot->SetPosition(Center - NodeSize * 0.5f);
			const bool bCurrentNode = FIntPoint(Room.mRow, Room.mColumn) == CurrentCoord;
			NodeSlot->SetZOrder(Room.mType == ERoomType::BossMonster ? 3 : (bCurrentNode ? 4 : 1));
		}
	}

	HideUnusedMapGraphWidgets(UsedLineCount, UsedNodeCount);
	CollapseOrphanGraphWidgets();
	UpdateOverlayMarkers(NodeCenters, CurrentCoord, SelectedCoord);

	if (bHasSelectedRoom)
	{
		SetMapPreviewText(SelectedRoomTitle, SelectedRoomDescription, SelectedRoomState, SelectedRoomStateColor);
	}
	else
	{
		SetMapPreviewText(FrontendMapText(TEXT("NoRoomSelected")), mMapReadyStatusText, FText::GetEmpty(), MutedColor);
	}

	if (EnterRoomButton != nullptr)
	{
		const bool bNavigationEnabled = IsFrontendMapNavigationEnabled();
		const bool bCanConfirmNextRoom = bNavigationEnabled && bHasSelectedRoom;
		SetEnterButtonVisible(bCanConfirmNextRoom);
		EnterRoomButton->SetIsEnabled(bCanConfirmNextRoom && !mEnterRequested);
	}

	SetEnterButtonText(mEnterText);
	SetMapStatusText(mStatusOverrideText.IsEmpty() ? mMapReadyStatusText : mStatusOverrideText);
	if (bShouldScrollToStart && MapScrollBox != nullptr)
	{
		MapScrollBox->ScrollToEnd();
	}
	else if (MapScrollBox != nullptr)
	{
		// 진입 가능/선택 노드가 없으면(전투 승리 직후 잠금 지도 등) 현재 위치 노드로 스크롤한다 — 최상단에 머무르지 않게.
		UWidget* ScrollTargetWidget = FocusMapNodeWidget != nullptr ? FocusMapNodeWidget : CurrentRoomNodeWidget;
		if (ScrollTargetWidget != nullptr)
		{
			MapScrollBox->ScrollWidgetIntoView(ScrollTargetWidget, false, EDescendantScrollDestination::Center, 48.f);
		}
	}
	return true;
}

void UFrontendMapWidget::SetPreviewRoomsForDebug(
	const TArray<FMapRoomView>& InRooms, bool bInAtStageStart)
{
	mDebugPreviewRooms = InRooms;
	mUseDebugPreviewRooms = true;
	mDebugPreviewAtStageStart = bInAtStageStart;
}

void UFrontendMapWidget::ClearPreviewRoomsForDebug()
{
	mDebugPreviewRooms.Reset();
	mUseDebugPreviewRooms = false;
	mDebugPreviewAtStageStart = false;
}

/**
 * @brief 지도 노드 클릭을 방 선택 GameMode API로 전달한다.
 *
 * @details
 * 지도는 선택 가능 여부를 UI에서 한 번 막고, 최종 유효성은 RoomGameMode의 SelectNextRoom()에 맡긴다.
 *
 * 왜 최종 검증을 GameMode에 맡기는가:
 * UI는 표시된 노드를 보고 입력을 막을 수 있지만, 런 진행 중 전환 요청이나 저장 상태 같은 게임 규칙은 알 수 없다.
 * GameMode가 마지막으로 검사해야 잘못된 노드 클릭이 실제 방 선택으로 이어지지 않는다.
 */
void UFrontendMapWidget::HandleMapRoomClicked(int32 RowIndex, int32 ColumnIndex)
{
	if (mEnterRequested)
	{
		return;
	}

	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		if (RoomGameMode->SelectNextRoom(RowIndex, ColumnIndex))
		{
			RefreshMap();
			SetMapStatusText(mStatusOverrideText.IsEmpty() ? mMapReadyStatusText : mStatusOverrideText);
		}
	}
}

/**
 * @brief 노드 위젯의 클릭 이벤트를 지도 선택 처리로 연결한다.
 */
void UFrontendMapWidget::HandleMapNodeClicked(int32 RowIndex, int32 ColumnIndex)
{
	HandleMapRoomClicked(RowIndex, ColumnIndex);
}

/**
 * @brief 닫기 버튼 입력을 외부 닫기 요청 이벤트로 전달한다.
 */
void UFrontendMapWidget::HandleCloseButtonClicked()
{
	UE_LOG(LogRD, Display, TEXT("RD_WORLD_MAP_BACK_CLICK room_selection=%d"),
		mRoomSelectionEnabled ? 1 : 0);
	OnCloseRequested.Broadcast();
}

/**
 * @brief 선택된 방 입장을 요청하고 중복 입력을 막는다.
 *
 * @details
 * 실제 프리로드와 전환은 RoomGameMode가 수행한다.
 * UI는 요청 중 상태 문구와 버튼 라벨만 바꾸고, 실패하면 다시 입력 가능한 상태로 되돌린다.
 *
 * 왜 UI에서 직접 전환하지 않는가:
 * 방 전환은 프리로드, 페이드, 로딩 알림, 저장 상태와 함께 움직이는 게임 흐름이다.
 * 지도는 "입장하고 싶다"는 요청만 보내야 전환 정책이 한 곳에 남는다.
 */
void UFrontendMapWidget::HandleEnterRoomButtonClicked()
{
	if (mEnterRequested || !IsFrontendMapNavigationEnabled())
	{
		return;
	}

	mEnterRequested = true;
	/* 버튼 라벨은 입장 그대로 둔다. 진행 상태는 상태 문구만 알린다. */
	SetMapStatusText(mLoadingStatusText);

	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		if (RoomGameMode->EnterSelectedRoom())
		{
			return;
		}
	}

	mEnterRequested = false;
	SetMapStatusText(mMapUnavailableStatusText);
}

void UFrontendMapWidget::SetMapStatusText(const FText& InText) const
{
	/*
	 * 상태 문구 TextBlock이 WBP에서 빠진 경우도 허용한다.
	 * 지도 핵심 기능은 노드/선 표시이므로, 상태 문구가 없어도 앱이 바로 중단되지 않게 한다.
	 */
	if (MapStatusText != nullptr)
	{
		MapStatusText->SetText(InText);
	}
}

void UFrontendMapWidget::SetEnterButtonText(const FText& InText) const
{
	/* ENTER 버튼 라벨이 연결되어 있을 때만 현재 요청 상태를 표시한다. */
	if (EnterButtonText != nullptr)
	{
		EnterButtonText->SetText(InText);
	}
}

void UFrontendMapWidget::SetEnterButtonVisible(bool bVisible) const
{
	// 조회용 지도에서는 액션 판을 남기지 않는다. 다음 방 선택 흐름에서 실제 선택이
	// 확정된 순간에만 버튼과 라벨을 함께 노출한다.
	const ESlateVisibility ButtonVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	const ESlateVisibility TextVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (EnterRoomButton != nullptr)
	{
		EnterRoomButton->SetVisibility(ButtonVisibility);
	}
	if (EnterButtonText != nullptr)
	{
		EnterButtonText->SetVisibility(TextVisibility);
	}
}

void UFrontendMapWidget::SetMapPreviewText(const FText& Title, const FText& Description, const FText& State, const FSlateColor& StateColor) const
{
	/*
	 * 노드형 WBP에서는 노드 자체 표현을 우선하고, 미리보기 패널 텍스트는 숨긴다.
	 * 함수 시그니처를 유지하는 이유는 이후 프리뷰 디자인이 다시 살아나도 RefreshMap() 호출 구조를 바꾸지 않기 위해서다.
	 */
	(void)Title;
	(void)Description;
	(void)State;
	(void)StateColor;

	HideUnusedMapTextSurfaces();
}

void UFrontendMapWidget::HideUnusedMapTextSurfaces() const
{
	/*
	 * 새 지도 WBP에서는 큰 제목/프리뷰 패널을 쓰지 않는다.
	 * 기존 WBP에 남아 있는 TextBlock이 화면에 중복 노출되지 않도록 비우고 Collapsed 처리한다.
	 */
	if (MapTitleText != nullptr)
	{
		// 새 시안은 상단 제목을 표시한다. 위치/폰트/중앙정렬은 빌더가 WBP 슬롯에 세팅한다.
		MapTitleText->SetText(mMapText);
		MapTitleText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (MapPreviewPanel != nullptr)
	{
		MapPreviewPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapPreviewTitleText != nullptr)
	{
		MapPreviewTitleText->SetText(FText::GetEmpty());
		MapPreviewTitleText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapPreviewDescriptionText != nullptr)
	{
		MapPreviewDescriptionText->SetText(FText::GetEmpty());
		MapPreviewDescriptionText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapPreviewStateText != nullptr)
	{
		MapPreviewStateText->SetText(FText::GetEmpty());
		MapPreviewStateText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CloseButtonText != nullptr)
	{
		// 프레임 텍스처에는 문구가 없다 — 클래스 선택과 동일하게 라벨을 버튼 위에 표시한다.
		CloseButtonText->SetText(mCloseText);
		CloseButtonText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	// EnterButtonText는 SetEnterButtonVisible/SetEnterButtonText가 모드에 따라 관리한다(여기서 끄지 않는다).
	if (MapLegendScroll != nullptr)
	{
		MapLegendScroll->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapLegendList != nullptr)
	{
		MapLegendList->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapLegendTitle != nullptr)
	{
		MapLegendTitle->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MapDimBackground != nullptr)
	{
		MapDimBackground->SetBrushColor(TransparentPanelColor);
	}
	if (MapPaperPanel != nullptr)
	{
		MapPaperPanel->SetBrushColor(TransparentPanelColor);
	}
	if (MapPaperShadow != nullptr)
	{
		MapPaperShadow->SetBrushColor(TransparentPanelColor);
	}
}

/**
 * @brief 현재 지도 화면에서 실제 방 선택/입장 API를 호출해도 되는지 확인한다.
 *
 * @details
 * 선택 모드가 켜져 있고, 현재 월드의 GameMode가 방 선택 API를 제공할 때만 true를 반환한다.
 *
 * 왜 GameMode 존재까지 확인하는가:
 * 같은 위젯은 타이틀/프론트 화면에서도 배치될 수 있다. 방 GameMode가 없는 곳에서 노드 입력을 열면
 * 사용자는 선택 가능한 것처럼 보지만 실제로는 처리할 대상이 없다.
 */
bool UFrontendMapWidget::IsFrontendMapNavigationEnabled() const
{
	return mRoomSelectionEnabled && GetWorld() != nullptr && GetWorld()->GetAuthGameMode<ARoomGameModeBase>() != nullptr;
}

FVector2D UFrontendMapWidget::GetMapGraphContentSize() const
{
	if (IsLandscapeLayout())
	{
		const float Width = MapGraphSize != nullptr && MapGraphSize->GetWidthOverride() > 1.f
			? MapGraphSize->GetWidthOverride() : 1672.f;
		const float Height = MapGraphSize != nullptr && MapGraphSize->GetHeightOverride() > 1.f
			? MapGraphSize->GetHeightOverride() : 941.f;
		return FVector2D(Width, Height);
	}

	float GraphWidth = 0.f;
	if (MapScrollBox != nullptr)
	{
		const float ScrollBoxWidth = MapScrollBox->GetCachedGeometry().GetLocalSize().X;
		if (ScrollBoxWidth > 64.f)
		{
			GraphWidth = ScrollBoxWidth;
		}
	}

	if (GraphWidth <= 64.f)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				FVector2D ViewportSize = FVector2D::ZeroVector;
				ViewportClient->GetViewportSize(OUT ViewportSize);
				if (ViewportSize.X > 64.f)
				{
					GraphWidth = ViewportSize.X;
				}
			}
		}
	}

	if (GraphWidth <= 64.f)
	{
		GraphWidth = MapGraphFallbackWidth;
	}

	// 좌/우 꽉 채움: 상한 클램프 없이 뷰포트 폭을 그대로 쓴다(하한만 안전 확보).
	GraphWidth = FMath::Max(GraphWidth, 320.f);

	// 높이 정본 = 상하 여백 + 노드 + (행 수 - 1) x 행 간격. 행이 적으면 스크롤이 짧아질 뿐 간격은 일정하다.
	float LeftFrac = 0.f;
	float RightFrac = 0.f;
	float TopPx = 0.f;
	float BottomPx = 0.f;
	GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx, GraphWidth);

	float GraphHeight = GraphWidth * MapGraphFallbackAspect;
	if (mCachedRowCount > 0)
	{
		GraphHeight = TopPx + BottomPx + GetMapNodeSize().Y
			+ StaticCast<float>(FMath::Max(0, mCachedRowCount - 1)) * GetMapRowPitch();
	}

	// 화면보다 짧으면 양피지가 끊겨 보이므로 최소 한 화면은 채운다.
	if (MapScrollBox != nullptr)
	{
		const float ViewportHeight = MapScrollBox->GetCachedGeometry().GetLocalSize().Y;
		if (ViewportHeight > 64.f)
		{
			GraphHeight = FMath::Max(GraphHeight, ViewportHeight);
		}
	}
	return FVector2D(GraphWidth, GraphHeight);
}

FVector2D UFrontendMapWidget::GetMapNodeSize() const
{
	if (mResolvedNodeSize > 1.f)
	{
		return FVector2D(mResolvedNodeSize, mResolvedNodeSize);
	}
	return FVector2D(GetMapNodeDesignSize(), GetMapNodeDesignSize());
}

float UFrontendMapWidget::GetMapNodeDesignSize() const
{
	const float Size = (Map_NodeMetrics != nullptr && Map_NodeMetrics->GetWidthOverride() > 1.f)
		? Map_NodeMetrics->GetWidthOverride()
		: MapNodeFallbackSize;
	if (IsLandscapeLayout())
	{
		return Size;
	}
	// 간격(행/열 피치)은 배율을 곱하지 않는다 — 아이콘만 상대적으로 커져야 한다.
	return Size * FMath::Max(0.1f, mMapNodeSizeScale);
}

void UFrontendMapWidget::ResolveNodeMetrics(const TArray<FMapRoomView>& Rooms)
{
	int32 MaxColumn = 0;
	TArray<int32> UsedColumns;
	for (const FMapRoomView& Room : Rooms)
	{
		MaxColumn = FMath::Max(MaxColumn, Room.mColumn);
		UsedColumns.AddUnique(Room.mColumn);
	}
	UsedColumns.Sort();

	if (IsLandscapeLayout())
	{
		mResolvedNodeSize = 0.f;
		mResolvedRowPitch = 0.f;
		const float DesignSize = GetMapNodeDesignSize();
		const FVector2D GraphSize = GetMapGraphContentSize();
		float LeftFrac = 0.f;
		float RightFrac = 0.f;
		float TopPx = 0.f;
		float BottomPx = 0.f;
		GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx, GraphSize.X);
		const float UsableWidth = FMath::Max(1.f, (RightFrac - LeftFrac) * GraphSize.X);
		const float UsableHeight = FMath::Max(1.f, GraphSize.Y - TopPx - BottomPx);
		const float RowPitch = mCachedRowCount > 1
			? UsableWidth / StaticCast<float>(mCachedRowCount - 1)
			: UsableWidth;
		/*
		 * Stage 열 번호는 0,3,6처럼 듬성듬성할 수 있다. 최댓값을 열 개수로 쓰면
		 * 실제 다섯 레인을 열세 레인으로 오인해 노드를 48px까지 줄인다.
		 * 가로형 지도는 실제 사용 중인 레인 수로만 크기를 계산한다.
		 */
		const float ColumnPitch = UsedColumns.Num() > 1
			? UsableHeight / StaticCast<float>(UsedColumns.Num() - 1)
			: UsableHeight;
		mResolvedRowPitch = RowPitch;
		mResolvedNodeSize = FMath::Clamp(
			FMath::Min3(DesignSize, RowPitch * 0.78f, ColumnPitch * 0.72f),
			48.f, DesignSize);
		return;
	}

	/*
	 * 아이콘 크기를 "한 칸"에서 파생시키고, 행 간격은 화면 높이에 맞춰 편다.
	 *
	 * 예전에는 아이콘 크기(시안 마커 x 배율)와 열 간격(가용폭 / 열수)이 서로
	 * 모르는 채 계산됐다. 열 간격에는 상한만 있고 아이콘 크기와 묶인 하한이
	 * 없어서, 화면이 좁아지면 반드시 겹쳤다(720폭 실측: 간격 42.5 vs 아이콘 162).
	 *
	 * 행 간격도 마찬가지다. 층이 15에서 8로 줄면 시안 간격(176) 그대로는 노드가
	 * 화면 아래쪽에만 뭉치고 위쪽 양피지가 텅 빈다. 그래서 스크롤이 필요 없을
	 * 만큼 층이 적으면 남는 높이에 행을 고르게 편다.
	 */
	mResolvedNodeSize = 0.f;    // 아래 계산은 마커 기준으로 재야 하므로 확정값을 비운다
	mResolvedRowPitch = 0.f;

	const float DesignSize = GetMapNodeDesignSize();
	const FVector2D ProbeSize = GetMapGraphContentSize();
	if (MaxColumn <= 0 || ProbeSize.X <= 64.f)
	{
		mResolvedNodeSize = DesignSize;
		return;
	}

	float LeftFrac = 0.f;
	float RightFrac = 0.f;
	float TopPx = 0.f;
	float BottomPx = 0.f;
	GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx, ProbeSize.X);

	/*
	 * 가로: 한 열이 차지하는 칸 안에 아이콘이 들어가야 한다.
	 * 실제 배치는 열 간격 상한(GetMapColPitchMax)에도 걸리므로, 둘 중 좁은 쪽을
	 * 기준으로 잡아야 한다 -- 칸만 보고 키우면 상한에 걸린 간격보다 커져 겹친다.
	 */
	const float SpanPx = FMath::Max(0.f, (RightFrac - LeftFrac) * ProbeSize.X);
	const float SlotPitch = FMath::Min(GetMapColPitchMax(),
		SpanPx / StaticCast<float>(MaxColumn + 1));

	// 세로: 남는 높이에 행을 고르게 편다(시안 간격이 하한).
	const float MarkerRowPitch = GetMapRowPitch();
	float RowPitch = MarkerRowPitch;
	if (mCachedRowCount > 1)
	{
		const float UsableHeight = FMath::Max(0.f, ProbeSize.Y - TopPx - BottomPx);
		RowPitch = FMath::Max(MarkerRowPitch,
			(UsableHeight - DesignSize) / StaticCast<float>(mCachedRowCount - 1));
	}
	mResolvedRowPitch = RowPitch;

	const float FitSize = FMath::Min(SlotPitch, RowPitch) * MapNodeSlotFillRatio;
	mResolvedNodeSize = FMath::Clamp(FitSize,
		FMath::Min(MapNodeMinSizePx, DesignSize), DesignSize);
}

float UFrontendMapWidget::GetMapRowPitch() const
{
	if (mResolvedRowPitch > 1.f)
	{
		return mResolvedRowPitch;
	}
	return (Map_NodeMetrics != nullptr && Map_NodeMetrics->GetHeightOverride() > 1.f)
		? Map_NodeMetrics->GetHeightOverride()
		: MapRowPitchFallback;
}

float UFrontendMapWidget::GetMapColPitchMax() const
{
	const float MarkerPitchMax = (Map_ColPitch != nullptr && Map_ColPitch->GetWidthOverride() > 1.f)
		? Map_ColPitch->GetWidthOverride()
		: MapColPitchMaxFallback;
	/*
	 * 시안 상한(240)은 열이 적을 때 노드가 과하게 벌어지는 것을 막으려는 값인데,
	 * 아이콘을 키우면 이 상한이 아이콘보다 좁아져 다시 겹친다. 아이콘이 들어갈
	 * 만큼은 항상 벌어지도록 하한을 둔다.
	 */
	return FMath::Max(MarkerPitchMax, GetMapNodeDesignSize() * MapColPitchMinNodeRatio);
}

void UFrontendMapWidget::GetNodeAreaLayout(float& OutLeftFrac, float& OutRightFrac, float& OutTopPx, float& OutBottomPx, float InGraphWidth) const
{
	if (IsLandscapeLayout())
	{
		OutLeftFrac = 0.19f;
		OutRightFrac = 0.91f;
		OutTopPx = 180.f;
		OutBottomPx = 115.f;
		if (const UCanvasPanelSlot* AreaSlot = Map_NodeArea != nullptr
			? Cast<UCanvasPanelSlot>(Map_NodeArea->Slot) : nullptr)
		{
			const FAnchors Anchors = AreaSlot->GetAnchors();
			if (Anchors.Maximum.X > Anchors.Minimum.X)
			{
				OutLeftFrac = Anchors.Minimum.X;
				OutRightFrac = Anchors.Maximum.X;
			}
			const FMargin Offsets = AreaSlot->GetOffsets();
			OutTopPx = FMath::Max(0.f, Offsets.Top);
			OutBottomPx = FMath::Max(0.f, Offsets.Bottom);
		}
		return;
	}

	// 시안 마커: 앵커 X 분수 = 좌우 경계, 오프셋 Top/Bottom = 상하 여백(px). 에디터에서 박스를 옮기면 여기로 반영된다.
	OutLeftFrac = MapNodeAreaLeftFracFallback;
	OutRightFrac = MapNodeAreaRightFracFallback;
	OutTopPx = MapNodeAreaTopPxFallback;
	OutBottomPx = MapNodeAreaBottomPxFallback;

	if (Map_NodeArea == nullptr)
	{
		return;
	}
	const UCanvasPanelSlot* AreaSlot = Cast<UCanvasPanelSlot>(Map_NodeArea->Slot);
	if (AreaSlot == nullptr)
	{
		return;
	}
	const FAnchors Anchors = AreaSlot->GetAnchors();
	if (Anchors.Maximum.X > Anchors.Minimum.X)
	{
		OutLeftFrac = Anchors.Minimum.X;
		OutRightFrac = Anchors.Maximum.X;
	}
	const FMargin Offsets = AreaSlot->GetOffsets();
	OutTopPx = FMath::Max(0.f, Offsets.Top);
	OutBottomPx = FMath::Max(0.f, Offsets.Bottom);

	if (InGraphWidth <= 64.f)
	{
		return;
	}

	float LeftPx = OutLeftFrac * InGraphWidth;
	float RightPx = OutRightFrac * InGraphWidth;

	const float DecorLeftSafePx = FMath::Clamp(InGraphWidth * MapNodeAreaNarrowLeftSafeFrac, MapNodeAreaMinLeftSafePx, MapNodeAreaMaxLeftSafePx);
	const float DecorRightInsetPx = FMath::Clamp(InGraphWidth * MapNodeAreaNarrowRightInsetFrac, MapNodeAreaMinRightInsetPx, MapNodeAreaMaxRightInsetPx);
	LeftPx = FMath::Max(LeftPx, DecorLeftSafePx);
	RightPx = FMath::Min(RightPx, InGraphWidth - DecorRightInsetPx);

	if (Map_LegendGroup != nullptr)
	{
		if (const UCanvasPanelSlot* LegendSlot = Cast<UCanvasPanelSlot>(Map_LegendGroup->Slot))
		{
			const FAnchors LegendAnchors = LegendSlot->GetAnchors();
			if (FMath::IsNearlyEqual(LegendAnchors.Minimum.X, LegendAnchors.Maximum.X))
			{
				const FVector2D LegendSize = LegendSlot->GetSize();
				const FVector2D LegendPosition = LegendSlot->GetPosition();
				const FVector2D LegendAlignment = LegendSlot->GetAlignment();
				const float LegendSlotLeft = InGraphWidth * LegendAnchors.Minimum.X + LegendPosition.X - LegendSize.X * LegendAlignment.X;
				const float LegendSlotRight = LegendSlotLeft + LegendSize.X;
				const float LegendScale = GetFrontendMapLegendScale(InGraphWidth, mLegendRefWidth, mLegendMinScale);
				const float LegendVisualLeft = LegendSlotRight - LegendSize.X * LegendScale;
				const float RightBeforeLegendPx = LegendVisualLeft - MapNodeLegendGapPx;
				if (RightBeforeLegendPx > LeftPx + MapNodeAreaMinUsableWidthPx)
				{
					RightPx = FMath::Min(RightPx, RightBeforeLegendPx);
				}
			}
		}
	}

	if (RightPx < LeftPx + MapNodeAreaMinUsableWidthPx)
	{
		const float CenterPx = FMath::Clamp((LeftPx + RightPx) * 0.5f, MapNodeAreaMinUsableWidthPx * 0.5f, InGraphWidth - MapNodeAreaMinUsableWidthPx * 0.5f);
		LeftPx = CenterPx - MapNodeAreaMinUsableWidthPx * 0.5f;
		RightPx = CenterPx + MapNodeAreaMinUsableWidthPx * 0.5f;
	}

	/*
	 * 마지막으로 최소 폭을 보장한다. 위의 장식/범례 여백은 넓은 화면 기준이라
	 * 폰에서는 노드가 놓일 폭을 절반 가까이 깎는다. 지도 가독성이 범례 비침보다
	 * 우선이므로, 좁아진 경우 화면 중앙 기준으로 다시 펴 준다.
	 */
	const float MinSpanPx = InGraphWidth * MapNodeAreaMinSpanFrac;
	if (RightPx - LeftPx < MinSpanPx)
	{
		const float CenterPx = InGraphWidth * 0.5f;
		LeftPx = FMath::Max(MapNodeAreaEdgeInsetPx, CenterPx - MinSpanPx * 0.5f);
		RightPx = FMath::Min(InGraphWidth - MapNodeAreaEdgeInsetPx, CenterPx + MinSpanPx * 0.5f);
	}

	OutLeftFrac = FMath::Clamp(LeftPx / InGraphWidth, 0.f, 1.f);
	OutRightFrac = FMath::Clamp(RightPx / InGraphWidth, OutLeftFrac, 1.f);
}

FVector2D UFrontendMapWidget::GetMapRoomNodeCenter(const TArray<FMapRoomView>& Rooms, const FMapRoomView& Room, const FVector2D& GraphSize) const
{
	if (IsLandscapeLayout())
	{
		int32 MaxRow = 0;
		TArray<int32> UsedColumns;
		for (const FMapRoomView& Candidate : Rooms)
		{
			MaxRow = FMath::Max(MaxRow, Candidate.mRow);
			UsedColumns.AddUnique(Candidate.mColumn);
		}
		UsedColumns.Sort();

		float LeftFrac = 0.f;
		float RightFrac = 0.f;
		float TopPx = 0.f;
		float BottomPx = 0.f;
		GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx, GraphSize.X);
		const FVector2D NodeSize = GetMapNodeSize();
		const float Half = NodeSize.X * 0.5f;
		const float MinX = GraphSize.X * LeftFrac + Half;
		const float MaxX = GraphSize.X * RightFrac - Half;
		const float MinY = TopPx + Half;
		const float MaxY = GraphSize.Y - BottomPx - Half;
		const float XAlpha = MaxRow > 0
			? StaticCast<float>(Room.mRow) / StaticCast<float>(MaxRow) : 0.f;
		const float X = FMath::Lerp(MinX, MaxX, XAlpha);
		const int32 LaneIndex = UsedColumns.IndexOfByKey(Room.mColumn);
		float Y = UsedColumns.Num() > 1 && LaneIndex != INDEX_NONE
			? FMath::Lerp(MinY, MaxY,
				StaticCast<float>(LaneIndex) / StaticCast<float>(UsedColumns.Num() - 1))
			: (MinY + MaxY) * 0.5f;

		// 우하단 입장 버튼 영역으로 들어가는 마지막 하단 경로만 위로 휘어 시안처럼 버튼 위에서 합류시킨다.
		constexpr float ActionReserveStartX = 1150.f;
		constexpr float ActionReserveTargetY = 620.f;
		if (X > ActionReserveStartX && Y > ActionReserveTargetY && MaxX > ActionReserveStartX)
		{
			const float ReserveAlpha = FMath::Clamp(
				(X - ActionReserveStartX) / (MaxX - ActionReserveStartX), 0.f, 1.f);
			Y = FMath::Lerp(Y, ActionReserveTargetY, ReserveAlpha);
		}
		const float Jitter = FMath::Min(8.f, NodeSize.X * 0.08f);
		return FVector2D(
			FMath::Clamp(X + Room.mPositionOffsetRate.X * Jitter, MinX, MaxX),
			FMath::Clamp(Y + Room.mPositionOffsetRate.Y * Jitter, MinY, MaxY));
	}

	/*
	 * Stage row/column -> 캔버스 좌표. 배치 경계/간격의 정본은 시안 마커다:
	 * - X: Map_NodeArea 좌우 분수 안에서 열을 분배하되, 열 간격이 Map_ColPitch 상한을 넘지 않게 중앙으로 모은다(간격 과대 방지).
	 * - Y: 아래(시작)에서 위(보스)로, 행 간격 Map_NodeMetrics 고정.
	 * mPositionOffsetRate는 기계적 배열을 깨는 지터, 마지막에 Map_NodeArea 안으로 Clamp한다.
	 */
	int32 MaxColumn = 0;
	for (const FMapRoomView& Candidate : Rooms)
	{
		MaxColumn = FMath::Max(MaxColumn, Candidate.mColumn);
	}

	float LeftFrac = 0.f;
	float RightFrac = 0.f;
	float TopPx = 0.f;
	float BottomPx = 0.f;
	GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx, GraphSize.X);
	const FVector2D NodeSize = GetMapNodeSize();

	const float NodeHalfX = NodeSize.X * 0.5f;
	const float NodeHalfY = NodeSize.Y * 0.5f;

	const float GraphMinX = NodeHalfX;
	const float GraphMaxX = FMath::Max(GraphMinX, GraphSize.X - NodeHalfX);
	const float AreaMinX = FMath::Clamp(GraphSize.X * LeftFrac + NodeHalfX, GraphMinX, GraphMaxX);
	const float AreaMaxX = FMath::Clamp(GraphSize.X * RightFrac - NodeHalfX, GraphMinX, GraphMaxX);
	const float ClampMinX = FMath::Min(AreaMinX, AreaMaxX);
	const float ClampMaxX = FMath::Max(AreaMinX, AreaMaxX);
	const float AreaCenterX = (ClampMinX + ClampMaxX) * 0.5f;
	const float AvailWidth = FMath::Max(0.f, ClampMaxX - ClampMinX);
	const float ColPitch = MaxColumn <= 0
		? 0.f
		: FMath::Min(GetMapColPitchMax(), AvailWidth / StaticCast<float>(MaxColumn));
	const float X = AreaCenterX
		+ (StaticCast<float>(Room.mColumn) - StaticCast<float>(MaxColumn) * 0.5f) * ColPitch;

	const float GraphMinY = NodeHalfY;
	const float GraphMaxY = FMath::Max(GraphMinY, GraphSize.Y - NodeHalfY);
	const float AreaTopY = FMath::Clamp(TopPx + NodeHalfY, GraphMinY, GraphMaxY);
	const float AreaBottomY = FMath::Clamp(GraphSize.Y - BottomPx - NodeHalfY, GraphMinY, GraphMaxY);
	const float ClampMinY = FMath::Min(AreaTopY, AreaBottomY);
	const float ClampMaxY = FMath::Max(AreaTopY, AreaBottomY);

	const float Y = ClampMaxY - StaticCast<float>(Room.mRow) * GetMapRowPitch();

	/*
	 * 배치는 평평한 콘텐츠 좌표 그대로 둔다. 눕힌 원근은 리테이너 머티리얼이
	 * 화면에서 걸므로(노드 포함 한 렌더타겟), 여기서 사다리꼴 보정을 하면
	 * 이중 왜곡이 된다.
	 */
	/*
	 * 기계적인 격자 느낌을 깨는 흔들림. 다만 좌우로는 열 간격에서 아이콘을 뺀
	 * 여유 안에서만 흔든다 — 고정 18px로 흔들면 간격이 좁을 때 이웃과 겹친다
	 * (실측: 간격 42.5px에서 두 노드가 29px까지 붙었다).
	 */
	const float FreeGapX = FMath::Max(0.f, ColPitch - NodeSize.X);
	const float JitterX = FMath::Min(MapNodeJitterMaxPx, FreeGapX * 0.4f);
	const FVector2D Offset(
		Room.mPositionOffsetRate.X * JitterX,
		Room.mPositionOffsetRate.Y * MapNodeJitterMaxPx);
	return FVector2D(
		FMath::Clamp(X + Offset.X, ClampMinX, ClampMaxX),
		FMath::Clamp(Y + Offset.Y, ClampMinY, ClampMaxY));
}

void UFrontendMapWidget::ConfigureMapGraphLayout() const
{
	if (IsLandscapeLayout())
	{
		const FVector2D GraphSize = GetMapGraphContentSize();
		if (MapGraphSize != nullptr)
		{
			MapGraphSize->SetWidthOverride(GraphSize.X);
			MapGraphSize->SetHeightOverride(GraphSize.Y);
		}
		UpdateGraphDecorLayout(GraphSize);
		if (MapGraphCanvas != nullptr)
		{
			MapGraphCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);
		}
		return;
	}

	/*
	 * 지도 그래프는 ScrollBox 안의 세로 긴 Canvas로 다룬다.
	 * 가로는 현재 ScrollBox/Viewport 폭에 맞추고, 높이만 길게 잡아 위아래 스크롤로 탐색한다.
	 */
	const FVector2D GraphSize = GetMapGraphContentSize();
	UpdateGraphDecorLayout(GraphSize);

	// 범례 반응형 축소: 로그 실측상 고정 크기(429x599)가 좁은 화면(폭 1110)에서 39%를 덮는다.
	// 디자인 폭 / 기준 폭 비율로 통째로 줄이되(우측 중앙 피벗 - 오른쪽에 붙은 채 축소), 하한은 시안이 정한다.
	UpdateCloseButtonLayout();
	// 범례가 접혀 있어도 여닫기 단추 배치는 여기서 같이 갱신된다.
	if (mMapLegendPlate != nullptr)
	{
		// 새 범례 판은 크기를 직접 화면에 맞추므로 축소 트랜스폼을 걸지 않는다
		// (같이 걸면 두 번 줄어 폰에서 글자가 안 읽힌다).
		UpdateLegendPlateLayout();
	}
	else if (Map_LegendGroup != nullptr && mLegendRefWidth > KINDA_SMALL_NUMBER)
	{
		const float LegendScale = GetFrontendMapLegendScale(GraphSize.X, mLegendRefWidth, mLegendMinScale);
		FWidgetTransform LegendTransform;
		LegendTransform.Scale = FVector2D(LegendScale, LegendScale);
		Map_LegendGroup->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
		Map_LegendGroup->SetRenderTransform(LegendTransform);
	}
	if (MapGraphSize != nullptr)
	{
		MapGraphSize->SetWidthOverride(GraphSize.X);
		MapGraphSize->SetHeightOverride(GraphSize.Y);
	}
	if (MapScrollBox != nullptr)
	{
		MapScrollBox->SetOrientation(Orient_Vertical);
		MapScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		MapScrollBox->SetClipping(EWidgetClipping::ClipToBounds);
		// 원근 리테이너가 스크롤 박스를 감싸면 캔버스 슬롯은 리테이너 소유다.
		UWidget* ScrollOuter = (mMapPerspectiveRetainer != nullptr
				&& MapScrollBox->GetParent() == mMapPerspectiveRetainer)
			? StaticCast<UWidget*>(mMapPerspectiveRetainer)
			: StaticCast<UWidget*>(MapScrollBox);
		if (UCanvasPanelSlot* ScrollSlot = Cast<UCanvasPanelSlot>(ScrollOuter->Slot))
		{
			// 가로는 풀블리드, 세로는 팝업 밴드. WBP의 옛 탑바 인셋 값에
			// 의존하지 않도록 패키징에서도 항상 강제한다.
			ScrollSlot->SetAnchors(FAnchors(
				0.0f, MapPopupBandTopFrac, 1.0f, MapPopupBandBottomFrac));
			ScrollSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
			ScrollSlot->SetAlignment(FVector2D::ZeroVector);
			ScrollSlot->SetAutoSize(false);
			ScrollSlot->SetZOrder(0);
		}
	}
	if (mMapPerspectiveRetainer != nullptr)
	{
		// MID 는 Slate 페인트 후에야 생기므로 설치 시점에 못 넣었으면 여기서 재시도.
		// 머티리얼 기본값도 같은 수치라 못 넣어도 그림/입력은 어긋나지 않는다.
		if (UMaterialInstanceDynamic* EffectMID = mMapPerspectiveRetainer->GetEffectMaterial())
		{
			EffectMID->SetScalarParameterValue(TEXT("TopWidth"), MapPerspectiveTopWidth);
		}
	}
	if (Map_Scrim != nullptr)
	{
		if (UCanvasPanelSlot* ScrimSlot = Cast<UCanvasPanelSlot>(Map_Scrim->Slot))
		{
			/*
			 * 정사각 배경 cover-fit: 짧은 변에 맞추면 긴 변 쪽에 빈 곳이 생기므로
			 * 긴 변 기준으로 키우고 중앙 정렬해 넘치는 쪽을 잘라낸다.
			 * 세로 화면이면 좌우가, 가로 화면이면 상하가 잘린다.
			 */
			FVector2D ScreenSize = GetCachedGeometry().GetLocalSize();
			if (ScreenSize.X <= 64.f || ScreenSize.Y <= 64.f)
			{
				ScreenSize = MapScrollBox != nullptr
					? MapScrollBox->GetCachedGeometry().GetLocalSize()
					: FVector2D::ZeroVector;
			}
			const float Side = FMath::Max(
				FMath::Max(StaticCast<float>(ScreenSize.X), StaticCast<float>(ScreenSize.Y)),
				64.f);
			ScrimSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			ScrimSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ScrimSlot->SetAutoSize(false);
			ScrimSlot->SetPosition(FVector2D::ZeroVector);
			ScrimSlot->SetSize(FVector2D(Side, Side));
			ScrimSlot->SetZOrder(-1000);
		}
	}
	if (MapGraphCanvas != nullptr)
	{
		MapGraphCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	}
}

void UFrontendMapWidget::UpdateGraphDecorLayout(const FVector2D& GraphSize) const
{
	/*
	 * 양피지 몸통/두루마리 로드는 WBP(시안 빌더) 소유다.
	 * 그래프 콘텐츠 높이가 행 수에 따라 매번 달라지므로, C++은 슬롯 치수만 콘텐츠에 동기한다.
	 */
	if (Map_ParchmentBody != nullptr)
	{
		if (UCanvasPanelSlot* BodySlot = Cast<UCanvasPanelSlot>(Map_ParchmentBody->Slot))
		{
			BodySlot->SetAnchors(FAnchors(0.f, 0.f));
			BodySlot->SetAlignment(FVector2D::ZeroVector);
			BodySlot->SetAutoSize(false);
			BodySlot->SetPosition(FVector2D::ZeroVector);
			BodySlot->SetSize(GraphSize);
			BodySlot->SetZOrder(-200);
		}
		Map_ParchmentBody->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 베일은 양피지와 같은 자리를 덮어야 한다(노드는 그 위에 남는다).
	if (mMapParchmentVeil != nullptr)
	{
		if (UCanvasPanelSlot* VeilSlot = Cast<UCanvasPanelSlot>(mMapParchmentVeil->Slot))
		{
			VeilSlot->SetAnchors(FAnchors(0.f, 0.f));
			VeilSlot->SetAlignment(FVector2D::ZeroVector);
			VeilSlot->SetAutoSize(false);
			VeilSlot->SetPosition(FVector2D::ZeroVector);
			VeilSlot->SetSize(GraphSize);
			VeilSlot->SetZOrder(-150);
		}
		mMapParchmentVeil->SetColorAndOpacity(mMapParchmentVeilColor);
		// 지도 그림 자체가 이미 눌려 있으면 베일은 필요 없다(이중으로 누르지 않는다).
		mMapParchmentVeil->SetVisibility(mMapParchmentVeilColor.A > 0.01f
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (Map_ScrollRodTop != nullptr)
	{
		if (UCanvasPanelSlot* RodSlot = Cast<UCanvasPanelSlot>(Map_ScrollRodTop->Slot))
		{
			const float RodHeight = RodSlot->GetSize().Y;   // 높이는 시안(빌더) 소유
			RodSlot->SetAnchors(FAnchors(0.f, 0.f));
			RodSlot->SetAlignment(FVector2D::ZeroVector);
			RodSlot->SetAutoSize(false);
			RodSlot->SetPosition(FVector2D::ZeroVector);
			RodSlot->SetSize(FVector2D(GraphSize.X, RodHeight));
			RodSlot->SetZOrder(-190);
		}
	}

	if (Map_ScrollRodBottom != nullptr)
	{
		if (UCanvasPanelSlot* RodSlot = Cast<UCanvasPanelSlot>(Map_ScrollRodBottom->Slot))
		{
			const float RodHeight = RodSlot->GetSize().Y;
			RodSlot->SetAnchors(FAnchors(0.f, 0.f));
			RodSlot->SetAlignment(FVector2D::ZeroVector);
			RodSlot->SetAutoSize(false);
			RodSlot->SetPosition(FVector2D(0.f, GraphSize.Y - RodHeight));
			RodSlot->SetSize(FVector2D(GraphSize.X, RodHeight));
			RodSlot->SetZOrder(-190);
		}
	}
}

void UFrontendMapWidget::UpdateOverlayMarkers(const TMap<FIntPoint, FVector2D>& NodeCenters, const FIntPoint& CurrentCoord, const FIntPoint& SelectedCoord) const
{
	const FVector2D* CurrentCenter = NodeCenters.Find(CurrentCoord);

	// 가로형 지도는 현재 파티 초상을 노드 중심에 겹치고, 별도 청색 오라로 위치를 강조한다.
	if (Map_CurrentAura != nullptr)
	{
		UCanvasPanelSlot* AuraSlot = Cast<UCanvasPanelSlot>(Map_CurrentAura->Slot);
		if (CurrentCenter != nullptr && AuraSlot != nullptr)
		{
			const FVector2D AuraSize = AuraSlot->GetSize();
			AuraSlot->SetPosition(*CurrentCenter - AuraSize * 0.5f);
			AuraSlot->SetZOrder(2);
			Map_CurrentAura->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Map_CurrentAura->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 기존 세로형은 화살표를 머리 위에, 가로형은 초상을 노드 중심에 배치한다.
	if (Map_CurrentMarker != nullptr)
	{
		UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Map_CurrentMarker->Slot);
		if (CurrentCenter != nullptr && MarkerSlot != nullptr)
		{
			const FVector2D MarkerSize = MarkerSlot->GetSize();
			MarkerSlot->SetPosition(IsLandscapeLayout()
				? *CurrentCenter - MarkerSize * 0.5f
				: FVector2D(
					CurrentCenter->X - MarkerSize.X * 0.5f,
					CurrentCenter->Y - GetMapNodeSize().Y * 0.5f - MarkerSize.Y));
			MarkerSlot->SetZOrder(7);
			Map_CurrentMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Map_CurrentMarker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 선택 글로우: 선택된 노드 뒤(선과 같은 층)에서 강조한다.
	if (Map_SelectGlow != nullptr)
	{
		const FVector2D* SelectedCenter = NodeCenters.Find(SelectedCoord);
		UCanvasPanelSlot* GlowSlot = Cast<UCanvasPanelSlot>(Map_SelectGlow->Slot);
		if (SelectedCenter != nullptr && GlowSlot != nullptr)
		{
			const FVector2D GlowSize = GlowSlot->GetSize();
			GlowSlot->SetPosition(*SelectedCenter - GlowSize * 0.5f);
			GlowSlot->SetZOrder(0);
			Map_SelectGlow->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Map_SelectGlow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

