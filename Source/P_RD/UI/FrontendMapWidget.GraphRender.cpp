#include "UI/FrontendMapWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "UI/FrontendMapGraphWidgets.h"
#include "UI/FrontendMapViewPolicy.h"

using namespace RDFrontendMap;

int32 UFrontendMapWidget::RefreshMapConnectionLines(const TArray<FMapRoomView>& Rooms, const TMap<FIntPoint, FVector2D>& NodeCenters)
{
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

			FMapConnectionLayout ConnectionLayout;
			if (BuildMapConnectionLayout(*FromCenter, *ToCenter, OUT ConnectionLayout) == false)
			{
				continue;
			}

			FFrontendMapLinePoolEntry* LineEntry = AcquireMapLineWidget(UsedLineCount++);
			if (LineEntry == nullptr || LineEntry->mLineWidget == nullptr)
			{
				continue;
			}

			UFrontendMapLineWidget* ConnectionLine = LineEntry->mLineWidget;
			ConnectionLine->SetLineColor(GetMapConnectionLineColor(Room));

			FWidgetTransform LineTransform;
			LineTransform.Angle = ConnectionLayout.mAngleDegrees;
			ConnectionLine->SetRenderTransform(LineTransform);

			if (UCanvasPanelSlot* LineSlot = Cast<UCanvasPanelSlot>(ConnectionLine->Slot))
			{
				LineSlot->SetSize(ConnectionLayout.mSize);
				LineSlot->SetPosition(ConnectionLayout.mPosition);
				LineSlot->SetZOrder(0);
			}
		}
	}
	return UsedLineCount;
}

int32 UFrontendMapWidget::RefreshMapRoomNodes(
	const TArray<FMapRoomView>& Rooms,
	bool& bOutHasSelectedRoom,
	FText& OutSelectedRoomTitle,
	FText& OutSelectedRoomDescription,
	FText& OutSelectedRoomState,
	FSlateColor& OutSelectedRoomStateColor,
	UWidget*& OutFocusMapNodeWidget)
{
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
			bOutHasSelectedRoom = true;
			OutSelectedRoomTitle = Room.mTitle;
			OutSelectedRoomDescription = Room.mDescription;
			OutSelectedRoomState = StateText;
			OutSelectedRoomStateColor = StateColor;
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
			FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.f)));
		if (bShouldFocusRoom)
		{
			OutFocusMapNodeWidget = NodeWidget;
		}

		const FVector2D Center = GetMapRoomNodeCenter(Rooms, Room);
		if (UCanvasPanelSlot* NodeSlot = Cast<UCanvasPanelSlot>(NodeWidget->Slot))
		{
			NodeSlot->SetSize(FVector2D(MapNodeWidth, MapNodeHeight));
			NodeSlot->SetPosition(GetMapRoomNodeTopLeft(Center));
			NodeSlot->SetZOrder(1);
		}
	}
	return UsedNodeCount;
}
