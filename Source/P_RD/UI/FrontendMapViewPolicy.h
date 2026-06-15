#pragma once

#include "RDMinimal.h"
#include "UI/RoomViewTypes.h"

namespace RDFrontendMap
{
	constexpr float MapGraphWidth = 1000.f;
	constexpr float MapGraphHeight = 1800.f;
	constexpr float MapNodeWidth = 132.f;
	constexpr float MapNodeHeight = 44.f;
	constexpr float MapGraphSidePadding = 96.f;
	constexpr float MapGraphTopPadding = 88.f;

	static const FLinearColor PanelDarkColor(0.215f, 0.240f, 0.260f, 1.f);
	static const FLinearColor AccentFillColor(0.255f, 0.565f, 0.590f, 1.f);
	static const FLinearColor SelectFillColor(0.640f, 0.545f, 0.345f, 1.f);
	static const FSlateColor TitleColor(FLinearColor(0.94f, 0.95f, 0.93f, 1.f));
	static const FSlateColor MutedColor(FLinearColor(0.63f, 0.67f, 0.69f, 1.f));
	static const FSlateColor ReadyColor(FLinearColor(0.53f, 0.86f, 0.88f, 1.f));
	static const FSlateColor LockedColor(FLinearColor(0.52f, 0.54f, 0.55f, 1.f));

	struct P_RD_API FMapConnectionLayout
	{
		FVector2D mPosition = FVector2D::ZeroVector;
		FVector2D mSize = FVector2D::ZeroVector;
		float mAngleDegrees = 0.f;
	};

	P_RD_API FText FrontendMapText(const TCHAR* Key);
	P_RD_API FLinearColor GetMapRoomTypeColor(ERoomType RoomType);
	P_RD_API FLinearColor GetMapRoomPanelColor(const FMapRoomView& Room);
	P_RD_API FLinearColor GetMapConnectionLineColor(const FMapRoomView& FromRoom);
	P_RD_API FSlateColor GetMapRoomTextColor(const FMapRoomView& Room);
	P_RD_API FText GetMapRoomBadgeText(const FMapRoomView& Room);
	P_RD_API FText GetMapRoomNodeLabel(const FMapRoomView& Room);
	P_RD_API FText GetMapRoomStateText(const FMapRoomView& Room);
	P_RD_API FText GetRoomDebugTypeText(ERoomType RoomType);
	P_RD_API FText GetMapRoomDebugNodeLabel(const FMapRoomView& Room);
	P_RD_API FVector2D GetMapRoomNodeCenter(const TArray<FMapRoomView>& Rooms, const FMapRoomView& Room);
	P_RD_API FVector2D GetMapRoomNodeTopLeft(const FVector2D& Center);
	P_RD_API bool BuildMapConnectionLayout(const FVector2D& FromCenter, const FVector2D& ToCenter, FMapConnectionLayout& OutLayout);
	P_RD_API const FMapRoomView* FindMapRoom(const TArray<FMapRoomView>& Rooms, int32 RowIndex, int32 ColumnIndex);
}
