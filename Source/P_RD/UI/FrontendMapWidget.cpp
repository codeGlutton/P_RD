#include "UI/FrontendMapWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "GameMode/RoomGameModeBase.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/ViewportZOrderType.h"
#include "UObject/ConstructorHelpers.h"

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
		return Room.mState == EFrontendMapRoomState::Locked ? LockedColor : TitleColor;
	}

	FText GetMapRoomBadgeText(const FFrontendMapRoomView& Room)
	{
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
		return Rooms.FindByPredicate([RowIndex, ColumnIndex](const FFrontendMapRoomView& Room)
		{
			return Room.mRow == RowIndex && Room.mColumn == ColumnIndex;
		});
	}
}

/**
 * @brief 월드맵 기본 WBP 클래스와 팝업 ZOrder를 초기화한다.
 *
 * @details
 * WBP_FrontendMap의 클래스 기본값이 비어 있어도 선/노드 WBP를 찾을 수 있도록 C++ fallback을 둔다.
 * 이 위젯은 탑바에서 OpenUI()로 열리는 팝업이므로 일반 HUD보다 위에 표시한다.
 */
UFrontendMapWidget::UFrontendMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UFrontendMapLineWidget> MapLineWidgetFinder(TEXT("/Game/BP/UI/WBP_FrontendMapLine"));
	if (MapLineWidgetFinder.Succeeded())
	{
		MapLineWidgetClass = MapLineWidgetFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UFrontendMapNodeWidget> MapNodeWidgetFinder(TEXT("/Game/BP/UI/WBP_FrontendMapNode"));
	if (MapNodeWidgetFinder.Succeeded())
	{
		MapNodeWidgetClass = MapNodeWidgetFinder.Class;
	}

	mViewportZOrder = static_cast<int32>(EViewportZOrderType::PopUp);
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
 * @brief 외부 상태 문구를 해제한다.
 */
void UFrontendMapWidget::ClearMapStatusOverride()
{
	mStatusOverrideText = FText::GetEmpty();
}

void UFrontendMapWidget::ValidateDesignerBindings() const
{
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
	else
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: RoomGameMode is not available. Map view data must be provided by RoomGameMode."));
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
	if (MapStatusText != nullptr)
	{
		MapStatusText->SetText(InText);
	}
}

void UFrontendMapWidget::SetEnterButtonText(const FText& InText) const
{
	if (EnterButtonText != nullptr)
	{
		EnterButtonText->SetText(InText);
	}
}

void UFrontendMapWidget::SetMapPreviewText(const FText& Title, const FText& Description, const FText& State, const FSlateColor& StateColor) const
{
	(void)Title;
	(void)Description;
	(void)State;
	(void)StateColor;

	HideUnusedMapTextSurfaces();
}

void UFrontendMapWidget::HideUnusedMapTextSurfaces() const
{
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
 */
bool UFrontendMapWidget::IsFrontendMapNavigationEnabled() const
{
	return bRoomSelectionEnabled && GetWorld() != nullptr && GetWorld()->GetAuthGameMode<ARoomGameModeBase>() != nullptr;
}

void UFrontendMapWidget::ConfigureMapGraphLayout() const
{
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
