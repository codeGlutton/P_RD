#include "UI/FrontendMapWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameMode/RoomGameModeBase.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/ViewportZOrderType.h"

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

	const FLinearColor PanelDarkColor(0.215f, 0.240f, 0.260f, 1.f);
	const FLinearColor AccentFillColor(0.255f, 0.565f, 0.590f, 1.f);
	const FLinearColor SelectFillColor(0.640f, 0.545f, 0.345f, 1.f);
	const FSlateColor TitleColor(FLinearColor(0.94f, 0.95f, 0.93f, 1.f));
	const FSlateColor MutedColor(FLinearColor(0.63f, 0.67f, 0.69f, 1.f));
	const FSlateColor ReadyColor(FLinearColor(0.53f, 0.86f, 0.88f, 1.f));
	const FSlateColor LockedColor(FLinearColor(0.52f, 0.54f, 0.55f, 1.f));
	const FLinearColor TransparentPanelColor(0.f, 0.f, 0.f, 0.f);

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
	// 배경/범례/버튼 스킨은 WBP(시안 빌더) 소유가 됐다 — C++ 런타임 생성/스타일링을 하지 않는다.
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
	mMapDragScrolling = false;
	UnbindEvents();
	for (FFrontendMapNodePoolEntry& NodeEntry : mMapNodePool)
	{
		if (NodeEntry.mNodeWidget != nullptr)
		{
			NodeEntry.mNodeWidget->OnMapNodeClicked.RemoveDynamic(this, &UFrontendMapWidget::HandleMapNodeClicked);
		}
	}
	mMapLinePool.Reset();
	mMapNodePool.Reset();
	Super::NativeDestruct();
}

void UFrontendMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	/*
	 * 뷰포트가 바뀌면(창 리사이즈/회전) 그래프 폭·높이와 노드 배치가 낡은 값으로 남는다.
	 * 몇 프레임 안정된 뒤 전체 리프레시로 다시 깔고, 조정용 메트릭 로그를 남긴다.
	 */
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (UGameViewportClient* ViewportClient = GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr)
	{
		ViewportClient->GetViewportSize(OUT ViewportSize);
	}
	if (!ViewportSize.Equals(mMetricsLastViewport, 0.5f))
	{
		mMetricsLastViewport = ViewportSize;
		mMetricsCountdown = 3;
	}
	else if (mMetricsCountdown > 0 && --mMetricsCountdown == 0)
	{
		RefreshMap();
		LogWorldMapMetrics();
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
		Entry.mNodeWidget->SetVisibility(ESlateVisibility::Visible);
	}
	return &Entry;
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
	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		bHasRooms = RoomGameMode->GetMapRoomViews(Rooms);

		FRunControlView RunControlView;
		bShouldScrollToStart = RoomGameMode->GetRunControlView(OUT RunControlView) && RunControlView.mIsAtStageStart;
	}
	else
	{
		UE_LOG(LogRD, Warning, TEXT("FrontendMapWidget: RoomGameMode is not available. WorldMap data must be provided by RoomGameMode."));
	}

	// 그래프 높이는 행 수 x 행 간격이므로, 데이터를 읽은 뒤에 레이아웃을 확정한다.
	int32 MaxRowIndex = 0;
	for (const FMapRoomView& Room : Rooms)
	{
		MaxRowIndex = FMath::Max(MaxRowIndex, Room.mRow);
	}
	mCachedRowCount = bHasRooms ? MaxRowIndex + 1 : 0;
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
	const FVector2D GraphSize = GetMapGraphContentSize();

	TMap<FIntPoint, FVector2D> NodeCenters;
	for (const FMapRoomView& Room : Rooms)
	{
		NodeCenters.Add(FIntPoint(Room.mRow, Room.mColumn), GetMapRoomNodeCenter(Rooms, Room, GraphSize));
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
			// 실선=열린 길 / 점선=잠긴 길 — 텍스처/틴트 모두 시안(WBP 클래스 디폴트) 소유.
			ConnectionLine->SetLineStyle(Room.mState != EMapRoomState::Locked);

			FWidgetTransform LineTransform;
			LineTransform.Angle = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			ConnectionLine->SetRenderTransform(LineTransform);

			const float LineThickness = ConnectionLine->GetLineThickness();
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

		const FVector2D NodeSize = GetMapNodeSize();
		const FVector2D Center = NodeCenters.FindChecked(FIntPoint(Room.mRow, Room.mColumn));
		if (UCanvasPanelSlot* NodeSlot = Cast<UCanvasPanelSlot>(NodeWidget->Slot))
		{
			NodeSlot->SetSize(NodeSize);
			NodeSlot->SetPosition(Center - NodeSize * 0.5f);
			NodeSlot->SetZOrder(1);
		}
	}

	HideUnusedMapGraphWidgets(UsedLineCount, UsedNodeCount);
	UpdateOverlayMarkers(NodeCenters, CurrentCoord, SelectedCoord);

	// [임시 진단] 노드/선 배치 덤프 — 해상도별 조정용. 배치 확정 후 제거.
	UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] refresh: nodes=%d lines=%d current=r%dc%d selected=r%dc%d"),
		UsedNodeCount, UsedLineCount, CurrentCoord.X, CurrentCoord.Y, SelectedCoord.X, SelectedCoord.Y);
	for (const FMapRoomView& Room : Rooms)
	{
		const FVector2D& Center = NodeCenters.FindChecked(FIntPoint(Room.mRow, Room.mColumn));
		UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] node r%dc%d %s %s center=(%.0f,%.0f)"),
			Room.mRow, Room.mColumn,
			*StaticEnum<ERoomType>()->GetNameStringByValue(StaticCast<int64>(Room.mType)),
			*StaticEnum<EMapRoomState>()->GetNameStringByValue(StaticCast<int64>(Room.mState)),
			Center.X, Center.Y);
	}

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
		SetEnterButtonVisible(bNavigationEnabled);
		EnterRoomButton->SetIsEnabled(bNavigationEnabled && bHasSelectedRoom && !mEnterRequested);
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
	SetEnterButtonText(mLoadingStatusText);
	SetMapStatusText(mLoadingStatusText);

	if (ARoomGameModeBase* RoomGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ARoomGameModeBase>() : nullptr)
	{
		if (RoomGameMode->EnterSelectedRoom())
		{
			return;
		}
	}

	mEnterRequested = false;
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

void UFrontendMapWidget::SetEnterButtonVisible(bool bVisible) const
{
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
	GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx);

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
	const float Size = (Map_NodeMetrics != nullptr && Map_NodeMetrics->GetWidthOverride() > 1.f)
		? Map_NodeMetrics->GetWidthOverride()
		: MapNodeFallbackSize;
	return FVector2D(Size, Size);
}

float UFrontendMapWidget::GetMapRowPitch() const
{
	return (Map_NodeMetrics != nullptr && Map_NodeMetrics->GetHeightOverride() > 1.f)
		? Map_NodeMetrics->GetHeightOverride()
		: MapRowPitchFallback;
}

float UFrontendMapWidget::GetMapColPitchMax() const
{
	return (Map_ColPitch != nullptr && Map_ColPitch->GetWidthOverride() > 1.f)
		? Map_ColPitch->GetWidthOverride()
		: MapColPitchMaxFallback;
}

void UFrontendMapWidget::GetNodeAreaLayout(float& OutLeftFrac, float& OutRightFrac, float& OutTopPx, float& OutBottomPx) const
{
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
}

FVector2D UFrontendMapWidget::GetMapRoomNodeCenter(const TArray<FMapRoomView>& Rooms, const FMapRoomView& Room, const FVector2D& GraphSize) const
{
	/*
	 * Stage row/column -> 캔버스 좌표. 배치 경계/간격의 정본은 시안 마커다:
	 * - X: Map_NodeArea 좌우 분수 안에서 열을 분배하되, 열 간격이 Map_ColPitch 상한을 넘지 않게 중앙으로 모은다(간격 과대 방지).
	 * - Y: 아래(시작)에서 위(보스)로, 행 간격 Map_NodeMetrics 고정.
	 * mPositionOffsetRate는 기계적 배열을 깨는 지터, 마지막에 캔버스 밖으로 못 나가게 Clamp.
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
	GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx);
	const FVector2D NodeSize = GetMapNodeSize();

	const float AreaLeft = GraphSize.X * LeftFrac + NodeSize.X * 0.5f;
	const float AreaRight = GraphSize.X * RightFrac - NodeSize.X * 0.5f;
	const float AvailWidth = FMath::Max(0.f, AreaRight - AreaLeft);
	const float ColPitch = MaxColumn <= 0
		? 0.f
		: FMath::Min(GetMapColPitchMax(), AvailWidth / StaticCast<float>(MaxColumn));
	const float X = GraphSize.X * 0.5f
		+ (StaticCast<float>(Room.mColumn) - StaticCast<float>(MaxColumn) * 0.5f) * ColPitch;

	const float BottomCenterY = GraphSize.Y - BottomPx - NodeSize.Y * 0.5f;
	const float Y = BottomCenterY - StaticCast<float>(Room.mRow) * GetMapRowPitch();

	const FVector2D Offset(Room.mPositionOffsetRate.X * 18.f, Room.mPositionOffsetRate.Y * 18.f);
	return FVector2D(
		FMath::Clamp(X + Offset.X, NodeSize.X * 0.5f, GraphSize.X - NodeSize.X * 0.5f),
		FMath::Clamp(Y + Offset.Y, NodeSize.Y * 0.5f, GraphSize.Y - NodeSize.Y * 0.5f));
}

void UFrontendMapWidget::ConfigureMapGraphLayout() const
{
	/*
	 * 지도 그래프는 ScrollBox 안의 세로 긴 Canvas로 다룬다.
	 * 가로는 현재 ScrollBox/Viewport 폭에 맞추고, 높이만 길게 잡아 위아래 스크롤로 탐색한다.
	 */
	const FVector2D GraphSize = GetMapGraphContentSize();
	UpdateGraphDecorLayout(GraphSize);

	// 범례 반응형 축소: 로그 실측상 고정 크기(429x599)가 좁은 화면(폭 1110)에서 39%를 덮는다.
	// 디자인 폭 / 기준 폭 비율로 통째로 줄이되(우측 중앙 피벗 - 오른쪽에 붙은 채 축소), 하한은 시안이 정한다.
	if (Map_LegendGroup != nullptr && mLegendRefWidth > KINDA_SMALL_NUMBER)
	{
		const float LegendScale = FMath::Clamp(GraphSize.X / mLegendRefWidth, mLegendMinScale, 1.f);
		Map_LegendGroup->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
		FWidgetTransform LegendTransform;
		LegendTransform.Scale = FVector2D(LegendScale, LegendScale);
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
		if (UCanvasPanelSlot* ScrollSlot = Cast<UCanvasPanelSlot>(MapScrollBox->Slot))
		{
			ScrollSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			// 탑바 인셋: 위 영역을 비워 전투 HUD 탑바가 보이고 눌리게 한다.
			ScrollSlot->SetOffsets(FMargin(0.0f, FMath::Max(0.f, mTopUIInset), 0.0f, 0.0f));
			ScrollSlot->SetAlignment(FVector2D::ZeroVector);
			ScrollSlot->SetAutoSize(false);
			ScrollSlot->SetZOrder(0);
		}
	}
	if (Map_Scrim != nullptr)
	{
		if (UCanvasPanelSlot* ScrimSlot = Cast<UCanvasPanelSlot>(Map_Scrim->Slot))
		{
			ScrimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			ScrimSlot->SetOffsets(FMargin(0.0f, FMath::Max(0.f, mTopUIInset), 0.0f, 0.0f));
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
	// 현재 위치 마커: 시안 크기(슬롯) 유지, 현재 노드 머리 위에 얹는다.
	if (Map_CurrentMarker != nullptr)
	{
		const FVector2D* CurrentCenter = NodeCenters.Find(CurrentCoord);
		UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Map_CurrentMarker->Slot);
		if (CurrentCenter != nullptr && MarkerSlot != nullptr)
		{
			const FVector2D MarkerSize = MarkerSlot->GetSize();
			MarkerSlot->SetPosition(FVector2D(
				CurrentCenter->X - MarkerSize.X * 0.5f,
				CurrentCenter->Y - GetMapNodeSize().Y * 0.5f - MarkerSize.Y));
			MarkerSlot->SetZOrder(6);
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

void UFrontendMapWidget::LogWorldMapMetrics() const
{
	/* [임시 진단] 좌표는 전부 디자인 px(DPI 나눔) — 시안(1920x1080 기준)과 직접 비교 가능. */
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (UGameViewportClient* ViewportClient = GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr)
	{
		ViewportClient->GetViewportSize(OUT ViewportSize);
	}
	const float Dpi = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	const FVector2D DesignSize = ViewportSize / Dpi;

	UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] ==== viewport=(%.0f,%.0f) dpi=%.3f design=(%.1f,%.1f) topInset=%.0f rows=%d"),
		ViewportSize.X, ViewportSize.Y, Dpi, DesignSize.X, DesignSize.Y, mTopUIInset, mCachedRowCount);

	const FVector2D GraphSize = GetMapGraphContentSize();
	float LeftFrac = 0.f;
	float RightFrac = 0.f;
	float TopPx = 0.f;
	float BottomPx = 0.f;
	GetNodeAreaLayout(OUT LeftFrac, OUT RightFrac, OUT TopPx, OUT BottomPx);
	const FVector2D NodeSize = GetMapNodeSize();
	const float AreaLeft = GraphSize.X * LeftFrac + NodeSize.X * 0.5f;
	const float AreaRight = GraphSize.X * RightFrac - NodeSize.X * 0.5f;
	UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] graph: size=(%.0f,%.0f) rowPitch=%.0f colPitchMax=%.0f nodeSize=%.0f"),
		GraphSize.X, GraphSize.Y, GetMapRowPitch(), GetMapColPitchMax(), NodeSize.X);
	UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] nodeArea: fracX=%.3f..%.3f topPx=%.0f bottomPx=%.0f centerRangeX=%.0f..%.0f"),
		LeftFrac, RightFrac, TopPx, BottomPx, AreaLeft, AreaRight);
	if (MapScrollBox != nullptr)
	{
		UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] scroll: offset=%.0f end=%.0f viewportH=%.0f"),
			MapScrollBox->GetScrollOffset(), MapScrollBox->GetScrollOffsetOfEnd(),
			MapScrollBox->GetCachedGeometry().GetLocalSize().Y);
	}

	auto LogWidgetGeometry = [Dpi](const TCHAR* Label, const UWidget* Widget)
	{
		if (Widget == nullptr)
		{
			UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] widget %s: (미바인딩)"), Label);
			return;
		}
		const FGeometry& Geometry = Widget->GetCachedGeometry();
		const FVector2D Position = Geometry.GetAbsolutePosition() / Dpi;
		const FVector2D Size = Geometry.GetLocalSize();
		UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] widget %s: pos=(%.0f,%.0f) size=(%.0f,%.0f) vis=%s"),
			Label, Position.X, Position.Y, Size.X, Size.Y,
			*StaticEnum<ESlateVisibility>()->GetNameStringByValue(StaticCast<int64>(Widget->GetVisibility())));
	};
	LogWidgetGeometry(TEXT("MapScrollBox"), MapScrollBox);
	LogWidgetGeometry(TEXT("Map_ParchmentBody"), Map_ParchmentBody);
	LogWidgetGeometry(TEXT("Map_NodeArea"), Map_NodeArea);
	LogWidgetGeometry(TEXT("Map_CurrentMarker"), Map_CurrentMarker);
	LogWidgetGeometry(TEXT("Map_SelectGlow"), Map_SelectGlow);
	LogWidgetGeometry(TEXT("CloseButton"), CloseButton);
	LogWidgetGeometry(TEXT("EnterRoomButton"), EnterRoomButton);
	LogWidgetGeometry(TEXT("MapStatusText"), MapStatusText);
	LogWidgetGeometry(TEXT("MapTitleText"), MapTitleText);
	LogWidgetGeometry(TEXT("Map_LegendGroup"), Map_LegendGroup);
	if (Map_LegendGroup != nullptr)
	{
		UE_LOG(LogRD, Display, TEXT("[WorldMapMetrics] legendScale=%.3f (refW=%.0f min=%.2f)"),
			Map_LegendGroup->GetRenderTransform().Scale.X, mLegendRefWidth, mLegendMinScale);
	}
	if (WidgetTree != nullptr)
	{
		LogWidgetGeometry(TEXT("Map_Scrim"), WidgetTree->FindWidget(TEXT("Map_Scrim")));
	}
}

