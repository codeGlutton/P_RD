#include "UI/FrontendMapWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GameMode/FrontendGameMode.h"
#include "GameMode/RoomGameModeBase.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/ViewportZOrderType.h"

namespace
{
	constexpr float MapGraphWidth = 1000.f;
	constexpr float MapGraphHeight = 1800.f;
	constexpr float MapNodeWidth = 132.f;
	constexpr float MapNodeHeight = 44.f;
	constexpr float MapGraphSidePadding = 96.f;
	constexpr float MapGraphTopPadding = 88.f;

	const FLinearColor PanelDarkColor(0.215f, 0.240f, 0.260f, 1.f);
	const FLinearColor AccentFillColor(0.255f, 0.565f, 0.590f, 1.f);
	const FLinearColor SelectFillColor(0.640f, 0.545f, 0.345f, 1.f);
	const FSlateColor TitleColor(FLinearColor(0.94f, 0.95f, 0.93f, 1.f));
	const FSlateColor MutedColor(FLinearColor(0.63f, 0.67f, 0.69f, 1.f));
	const FSlateColor ReadyColor(FLinearColor(0.53f, 0.86f, 0.88f, 1.f));
	const FSlateColor LockedColor(FLinearColor(0.52f, 0.54f, 0.55f, 1.f));

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

	FLinearColor GetMapRoomPanelColor(const FFrontendMapRoomView& Room)
	{
		/*
		 * 방의 게임 상태를 지도 노드의 배경색으로 바꾼다.
		 * UI 위젯은 Ready/Selected/Cleared/Locked 같은 View 상태만 보고 색을 고르며,
		 * 실제 선택 가능 여부 판단은 GameMode에서 끝난 뒤 DTO로 내려온다.
		 */
		if (Room.bIsStartPoint)
		{
			return FLinearColor(0.245f, 0.300f, 0.315f, 1.f);
		}

		switch (Room.mState)
		{
		case EFrontendMapRoomState::Selected:
			return SelectFillColor;
		case EFrontendMapRoomState::Ready:
			return AccentFillColor;
		case EFrontendMapRoomState::Cleared:
			return FLinearColor(0.180f, 0.260f, 0.210f, 1.f);
		default:
			return PanelDarkColor;
		}
	}

	FSlateColor GetMapRoomTextColor(const FFrontendMapRoomView& Room)
	{
		/* 잠긴 노드는 텍스트도 흐리게 보여 "보이지만 아직 갈 수 없음"을 명확히 한다. */
		return Room.mState == EFrontendMapRoomState::Locked ? LockedColor : TitleColor;
	}

	FText GetMapRoomBadgeText(const FFrontendMapRoomView& Room)
	{
		/*
		 * 예전 카드형 지도 UI에서 쓰던 상태 배지 문구다.
		 * 현재 WBP에서는 일부 텍스트 표면을 숨기지만, 지도 노드/프리뷰 표현이 다시 필요해질 때 같은 상태 문구를 재사용한다.
		 */
		if (Room.bIsStartPoint)
		{
			return FrontendMapText(TEXT("MapStartBadge"));
		}

		switch (Room.mState)
		{
		case EFrontendMapRoomState::Selected:
			return FrontendMapText(TEXT("MapRoomSelectedBadge"));
		case EFrontendMapRoomState::Ready:
			return FrontendMapText(TEXT("MapRoomReadyBadge"));
		case EFrontendMapRoomState::Cleared:
			return FrontendMapText(TEXT("MapRoomVisitedBadge"));
		default:
			return FrontendMapText(TEXT("MapRoomLockedBadge"));
		}
	}

	FText GetMapRoomNodeLabel(const FFrontendMapRoomView& Room)
	{
		/*
		 * 최종 아이콘이 없을 때 노드 안에 표시할 최소 라벨이다.
		 * 시작점은 START, 일반 방은 행-열 형식으로 표시해 생성된 Stage 구조를 확인한다.
		 */
		if (Room.bIsStartPoint)
		{
			return FrontendMapText(TEXT("MapStartNodeLabel"));
		}

		return FText::Format(
			FrontendMapText(TEXT("MapRoomCompactNodeLabelFormat")),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1));
	}

	FText GetMapRoomStateText(const FFrontendMapRoomView& Room)
	{
		/*
		 * 프리뷰 영역이나 접근성 텍스트에서 사용할 상태 문구다.
		 * 노드 클릭 가능 여부와 별개로, 사용자가 현재 방이 왜 어둡거나 밝은지 읽을 수 있게 한다.
		 */
		if (Room.bIsStartPoint)
		{
			return FrontendMapText(TEXT("MapStartState"));
		}

		switch (Room.mState)
		{
		case EFrontendMapRoomState::Selected:
			return FrontendMapText(TEXT("MapRoomSelected"));
		case EFrontendMapRoomState::Ready:
			return FrontendMapText(TEXT("MapRoomReady"));
		case EFrontendMapRoomState::Cleared:
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
	FText GetMapRoomDebugNodeLabel(const FFrontendMapRoomView& Room)
	{
		return FText::Format(
			FrontendMapText(TEXT("MapRoomDebugNodeLabelFormat")),
			FText::AsNumber(Room.mRow + 1),
			FText::AsNumber(Room.mColumn + 1),
			GetRoomDebugTypeText(Room.mType));
	}

	FVector2D GetMapRoomNodeCenter(const TArray<FFrontendMapRoomView>& Rooms, const FFrontendMapRoomView& Room)
	{
		/*
		 * Stage의 row/column을 고정 크기 지도 캔버스 좌표로 변환한다.
		 * X는 column 진행, Y는 row 진행을 뒤집어서 "위쪽이 다음 진행 방향"처럼 보이게 배치한다.
		 *
		 * mPositionOffsetRate는 같은 행의 노드가 너무 기계적으로 보이지 않도록 DataAsset/Stage 생성 결과에서 내려오는 작은 보정값이다.
		 * 최종 노드가 캔버스 밖으로 나가지 않게 마지막에 Clamp한다.
		 */
		int32 MaxRow = 0;
		int32 MaxColumn = 0;
		for (const FFrontendMapRoomView& Candidate : Rooms)
		{
			MaxRow = FMath::Max(MaxRow, Candidate.mRow);
			MaxColumn = FMath::Max(MaxColumn, Candidate.mColumn);
		}

		const float GraphRight = MapGraphWidth - MapGraphSidePadding;
		const float GraphBottom = MapGraphHeight - MapGraphTopPadding;
		const float X = MaxColumn <= 0
			? MapGraphWidth * 0.5f
			: MapGraphSidePadding + (static_cast<float>(Room.mColumn) / static_cast<float>(MaxColumn)) * (GraphRight - MapGraphSidePadding);
		const float Y = MaxRow <= 0
			? MapGraphHeight * 0.5f
			: MapGraphTopPadding + (static_cast<float>(MaxRow - Room.mRow) / static_cast<float>(MaxRow)) * (GraphBottom - MapGraphTopPadding);

		const FVector2D Offset(Room.mPositionOffsetRate.X * 18.f, Room.mPositionOffsetRate.Y * 18.f);
		return FVector2D(
			FMath::Clamp(X + Offset.X, MapNodeWidth * 0.5f, MapGraphWidth - MapNodeWidth * 0.5f),
			FMath::Clamp(Y + Offset.Y, MapNodeHeight * 0.5f, MapGraphHeight - MapNodeHeight * 0.5f));
	}

	const FFrontendMapRoomView* FindMapRoom(const TArray<FFrontendMapRoomView>& Rooms, int32 RowIndex, int32 ColumnIndex)
	{
		/*
		 * 선을 그릴 때 "현재 방의 다음 열"이 실제 방 View 배열 안에 있는지 찾는다.
		 * FStage 내부 구조를 지도 위젯이 직접 들고 있지 않기 때문에, DTO 배열 안에서 row/column으로만 연결한다.
		 */
		return Rooms.FindByPredicate([RowIndex, ColumnIndex](const FFrontendMapRoomView& Room)
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
 * 월드맵은 탑바에서 OpenUI()로 열리는 팝업이므로 일반 HUD보다 위에 표시한다.
 */
UFrontendMapWidget::UFrontendMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = StaticCast<int32>(EViewportZOrderType::PopUp);
	RefreshLocalizedTextCache();
}

/**
 * @brief 디자이너 바인딩과 버튼 이벤트를 준비하고 현재 지도 데이터를 그린다.
 */
void UFrontendMapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();
	BindEvents();
	ConfigureMapGraphLayout();
	RefreshLocalizedTextCache();
	HideUnusedMapTextSurfaces();
	RefreshMap();
}

/**
 * @brief 버튼/노드 이벤트 연결과 동적 생성한 그래프 풀을 정리한다.
 */
void UFrontendMapWidget::NativeDestruct()
{
	UnbindEvents();
	for (FFrontendMapNodePoolEntry& NodeEntry : MapNodePool)
	{
		if (NodeEntry.NodeWidget != nullptr)
		{
			NodeEntry.NodeWidget->OnMapNodeClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleMapNodeClicked);
		}
	}
	MapLinePool.Reset();
	MapNodePool.Reset();
	Super::NativeDestruct();
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
	bRoomSelectionEnabled = bEnabled;
}

/**
 * @brief 현재 지도 입력 허용 상태를 반환한다.
 */
bool UFrontendMapWidget::IsRoomSelectionEnabled() const
{
	return bRoomSelectionEnabled;
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
	if (MapScrollBox == nullptr)
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

	while (MapLinePool.Num() <= LineIndex)
	{
		FFrontendMapLinePoolEntry NewEntry;
		NewEntry.LineWidget = CreateWidget<UFrontendMapLineWidget>(this, MapLineWidgetClass);
		if (NewEntry.LineWidget != nullptr)
		{
			NewEntry.LineWidget->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
			NewEntry.LineWidget->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* LineSlot = MapGraphCanvas->AddChildToCanvas(NewEntry.LineWidget))
			{
				LineSlot->SetAutoSize(false);
				LineSlot->SetZOrder(0);
			}
		}
		MapLinePool.Add(MoveTemp(NewEntry));
	}

	FFrontendMapLinePoolEntry& Entry = MapLinePool[LineIndex];
	if (Entry.LineWidget != nullptr)
	{
		Entry.LineWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
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

	while (MapNodePool.Num() <= NodeIndex)
	{
		FFrontendMapNodePoolEntry NewEntry;
		NewEntry.NodeWidget = CreateWidget<UFrontendMapNodeWidget>(this, MapNodeWidgetClass);
		if (NewEntry.NodeWidget != nullptr)
		{
			NewEntry.NodeWidget->SetVisibility(ESlateVisibility::Collapsed);
			NewEntry.NodeWidget->OnMapNodeClicked.AddUniqueDynamic(this, &UFrontendMapWidget::HandleMapNodeClicked);

			if (UCanvasPanelSlot* NodeSlot = MapGraphCanvas->AddChildToCanvas(NewEntry.NodeWidget))
			{
				NodeSlot->SetAutoSize(false);
				NodeSlot->SetSize(FVector2D(MapNodeWidth, MapNodeHeight));
				NodeSlot->SetZOrder(1);
			}
		}
		MapNodePool.Add(MoveTemp(NewEntry));
	}

	FFrontendMapNodePoolEntry& Entry = MapNodePool[NodeIndex];
	if (Entry.NodeWidget != nullptr)
	{
		Entry.NodeWidget->SetVisibility(ESlateVisibility::Visible);
	}
	return &Entry;
}

void UFrontendMapWidget::HideUnusedMapGraphWidgets(int32 UsedLineCount, int32 UsedNodeCount)
{
	/*
	 * 이전 지도 갱신에서 더 많은 노드/선이 필요했다면 풀 안에 남은 위젯이 있을 수 있다.
	 * 이번 갱신에서 사용하지 않는 나머지는 제거하지 않고 숨겨 다음 갱신 때 재사용한다.
	 */
	for (int32 LineIndex = UsedLineCount; LineIndex < MapLinePool.Num(); ++LineIndex)
	{
		if (UFrontendMapLineWidget* LineWidget = MapLinePool[LineIndex].LineWidget)
		{
			LineWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	for (int32 NodeIndex = UsedNodeCount; NodeIndex < MapNodePool.Num(); ++NodeIndex)
	{
		if (UFrontendMapNodeWidget* NodeWidget = MapNodePool[NodeIndex].NodeWidget)
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
 * 같은 월드맵이 조회용과 다음 방 선택용으로 쓰이므로, bRoomSelectionEnabled와 bEnterRequested를 함께 보고
 * 노드/입장 버튼의 입력 가능 여부를 결정한다. 상태 문구는 전투 승리 같은 외부 흐름의 오버라이드가 있으면 그것을 우선한다.
 *
 * 왜 매번 View DTO를 다시 가져오는가:
 * 지도 노드를 클릭하면 선택 상태가 GameMode의 런 데이터에 반영되고, 그 결과로 Ready/Selected/Locked 표시가 바뀐다.
 * UI가 자체 캐시만 믿지 않고 다시 그리면 실제 런 상태와 화면 표시가 어긋나지 않는다.
 */
bool UFrontendMapWidget::RefreshMap()
{
	RefreshLocalizedTextCache();
	ConfigureMapGraphLayout();
	HideUnusedMapTextSurfaces();
	bEnterRequested = false;

	if (MapGraphCanvas == nullptr)
	{
		return false;
	}

	TArray<FFrontendMapRoomView> Rooms;
	bool bHasRooms = false;
	bool bShouldScrollToStart = false;
	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		bHasRooms = RoomGameMode->GetMapRoomViews(Rooms);

		FFrontendRunControlView RunControlView;
		bShouldScrollToStart = RoomGameMode->GetRunControlView(OUT RunControlView) && RunControlView.bIsAtStageStart;
	}
	else if (AFrontendGameMode* FrontendGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AFrontendGameMode>() : nullptr)
	{
		bHasRooms = FrontendGameMode->GetMapRoomViews(Rooms);

		FFrontendRunControlView RunControlView;
		bShouldScrollToStart = FrontendGameMode->GetRunControlView(OUT RunControlView) && RunControlView.bIsAtStageStart;
	}
	else
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: GameMode is not available. Map view data must be provided by FrontendGameMode or RoomGameMode."));
	}

	if (!bHasRooms)
	{
		HideUnusedMapGraphWidgets(0, 0);
		SetMapStatusText(mMapUnavailableStatusText);
		SetMapPreviewText(FrontendMapText(TEXT("NoRoomSelected")), mMapUnavailableStatusText, FText::GetEmpty(), MutedColor);
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

	TMap<FIntPoint, FVector2D> NodeCenters;
	for (const FFrontendMapRoomView& Room : Rooms)
	{
		NodeCenters.Add(FIntPoint(Room.mRow, Room.mColumn), GetMapRoomNodeCenter(Rooms, Room));
	}

	int32 UsedLineCount = 0;
	for (const FFrontendMapRoomView& Room : Rooms)
	{
		const FVector2D* FromCenter = NodeCenters.Find(FIntPoint(Room.mRow, Room.mColumn));
		if (FromCenter == nullptr)
		{
			continue;
		}

		for (int32 NextColumn : Room.mNextRoomColumns)
		{
			const FFrontendMapRoomView* NextRoom = FindMapRoom(Rooms, Room.mRow + 1, NextColumn);
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
			if (LineEntry == nullptr || LineEntry->LineWidget == nullptr)
			{
				continue;
			}

			UFrontendMapLineWidget* ConnectionLine = LineEntry->LineWidget;
			ConnectionLine->SetLineColor(Room.mState != EFrontendMapRoomState::Locked
				? FLinearColor(0.445f, 0.760f, 0.780f, 0.92f)
				: FLinearColor(0.250f, 0.300f, 0.320f, 0.58f));

			FWidgetTransform LineTransform;
			LineTransform.Angle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			ConnectionLine->SetRenderTransform(LineTransform);

			if (UCanvasPanelSlot* LineSlot = Cast<UCanvasPanelSlot>(ConnectionLine->Slot))
			{
				LineSlot->SetSize(FVector2D(Length, 4.f));
				LineSlot->SetPosition(*FromCenter - FVector2D(0.f, 2.f));
				LineSlot->SetZOrder(0);
			}
		}
	}

	int32 UsedNodeCount = 0;
	for (const FFrontendMapRoomView& Room : Rooms)
	{
		const FText StateText = GetMapRoomStateText(Room);
		const FSlateColor StateColor = Room.mState == EFrontendMapRoomState::Locked
			? LockedColor
			: (Room.mState == EFrontendMapRoomState::Selected ? TitleColor : ReadyColor);

		const bool bShouldFocusRoom = Room.bCanEnter || Room.bSelected;
		if (bShouldFocusRoom)
		{
			bHasSelectedRoom = true;
			SelectedRoomTitle = Room.mTitle;
			SelectedRoomDescription = Room.mDescription;
			SelectedRoomState = StateText;
			SelectedRoomStateColor = StateColor;
		}

		FFrontendMapNodePoolEntry* NodeEntry = AcquireMapNodeWidget(UsedNodeCount++);
		if (NodeEntry == nullptr || NodeEntry->NodeWidget == nullptr)
		{
			continue;
		}

		UFrontendMapNodeWidget* NodeWidget = NodeEntry->NodeWidget;
		NodeWidget->SetNodeEnabled(Room.bSelectable && IsFrontendMapNavigationEnabled() && !bEnterRequested);
		NodeWidget->SetNodeVisual(
			Room.mRow,
			Room.mColumn,
			GetMapRoomDebugNodeLabel(Room),
			FText::GetEmpty(),
			GetMapRoomPanelColor(Room),
			GetMapRoomTypeColor(Room.mType),
			GetMapRoomTextColor(Room),
			FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.f)));
		if (bShouldFocusRoom)
		{
			FocusMapNodeWidget = NodeWidget;
		}

		const FVector2D Center = GetMapRoomNodeCenter(Rooms, Room);
		if (UCanvasPanelSlot* NodeSlot = Cast<UCanvasPanelSlot>(NodeWidget->Slot))
		{
			NodeSlot->SetSize(FVector2D(MapNodeWidth, MapNodeHeight));
			NodeSlot->SetPosition(Center - FVector2D(MapNodeWidth * 0.5f, MapNodeHeight * 0.5f));
			NodeSlot->SetZOrder(1);
		}
	}

	HideUnusedMapGraphWidgets(UsedLineCount, UsedNodeCount);

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
		EnterRoomButton->SetIsEnabled(bHasSelectedRoom && IsFrontendMapNavigationEnabled() && !bEnterRequested);
	}

	SetEnterButtonText(mEnterText);
	SetMapStatusText(mStatusOverrideText.IsEmpty() ? mMapReadyStatusText : mStatusOverrideText);
	if (bShouldScrollToStart && MapScrollBox != nullptr)
	{
		MapScrollBox->ScrollToEnd();
	}
	else if (FocusMapNodeWidget != nullptr && MapScrollBox != nullptr)
	{
		MapScrollBox->ScrollWidgetIntoView(FocusMapNodeWidget, false, EDescendantScrollDestination::Center, 48.f);
	}
	return true;
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
	if (bEnterRequested)
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
	if (bEnterRequested)
	{
		return;
	}

	bEnterRequested = true;
	SetEnterButtonText(mLoadingStatusText);
	SetMapStatusText(mLoadingStatusText);

	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		if (RoomGameMode->EnterSelectedRoom())
		{
			return;
		}
	}

	bEnterRequested = false;
	SetEnterButtonText(mEnterText);
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

void UFrontendMapWidget::SetMapPreviewText(const FText& Title, const FText& Description, const FText& State, const FSlateColor& StateColor) const
{
	/*
	 * 새 WBP에서는 노드 자체 표현을 우선하고, 예전 미리보기 패널 텍스트는 숨긴다.
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
		MapTitleText->SetText(FText::GetEmpty());
		MapTitleText->SetVisibility(ESlateVisibility::Collapsed);
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
		CloseButtonText->SetText(mCloseText);
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
	return bRoomSelectionEnabled && GetWorld() != nullptr && GetWorld()->GetAuthGameMode<ARoomGameModeBase>() != nullptr;
}

void UFrontendMapWidget::ConfigureMapGraphLayout() const
{
	/*
	 * 지도 그래프는 ScrollBox 안의 고정 크기 Canvas로 다룬다.
	 * 노드 좌표 계산이 고정 캔버스 크기를 기준으로 하므로, WBP의 SizeBox 크기도 매번 같은 값으로 맞춘다.
	 */
	if (MapGraphSize != nullptr)
	{
		MapGraphSize->SetWidthOverride(MapGraphWidth);
		MapGraphSize->SetHeightOverride(MapGraphHeight);
	}
	if (MapScrollBox != nullptr)
	{
		MapScrollBox->SetOrientation(Orient_Vertical);
		MapScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		MapScrollBox->SetClipping(EWidgetClipping::ClipToBounds);
	}
	if (MapGraphCanvas != nullptr)
	{
		MapGraphCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	}
}
