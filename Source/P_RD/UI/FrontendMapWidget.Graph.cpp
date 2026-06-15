#include "UI/FrontendMapWidget.h"

#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "GameMode/RoomGameModeBase.h"
#include "UI/FrontendMapViewPolicy.h"

using namespace RDFrontendMap;

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
	ConfigureMapGraphLayout();
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
	for (const FMapRoomView& Room : Rooms)
	{
		NodeCenters.Add(FIntPoint(Room.mRow, Room.mColumn), GetMapRoomNodeCenter(Rooms, Room));
	}

	const int32 UsedLineCount = RefreshMapConnectionLines(Rooms, NodeCenters);
	const int32 UsedNodeCount = RefreshMapRoomNodes(
		Rooms,
		OUT bHasSelectedRoom,
		OUT SelectedRoomTitle,
		OUT SelectedRoomDescription,
		OUT SelectedRoomState,
		OUT SelectedRoomStateColor,
		OUT FocusMapNodeWidget);

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
		EnterRoomButton->SetIsEnabled(bHasSelectedRoom && IsFrontendMapNavigationEnabled() && !mEnterRequested);
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
