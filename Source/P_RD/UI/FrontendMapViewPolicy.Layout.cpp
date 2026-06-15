#include "UI/FrontendMapViewPolicy.h"

FVector2D RDFrontendMap::GetMapRoomNodeCenter(const TArray<FMapRoomView>& Rooms, const FMapRoomView& Room)
{
	int32 MaxRow = 0;
	int32 MaxColumn = 0;
	for (const FMapRoomView& Candidate : Rooms)
	{
		MaxRow = FMath::Max(MaxRow, Candidate.mRow);
		MaxColumn = FMath::Max(MaxColumn, Candidate.mColumn);
	}

	const float GraphRight = MapGraphWidth - MapGraphSidePadding;
	const float GraphBottom = MapGraphHeight - MapGraphTopPadding;
	const float X = MaxColumn <= 0
		? MapGraphWidth * 0.5f
		: MapGraphSidePadding + (StaticCast<float>(Room.mColumn) / StaticCast<float>(MaxColumn)) * (GraphRight - MapGraphSidePadding);
	const float Y = MaxRow <= 0
		? MapGraphHeight * 0.5f
		: MapGraphTopPadding + (StaticCast<float>(MaxRow - Room.mRow) / StaticCast<float>(MaxRow)) * (GraphBottom - MapGraphTopPadding);

	const FVector2D Offset(Room.mPositionOffsetRate.X * 18.f, Room.mPositionOffsetRate.Y * 18.f);
	return FVector2D(
		FMath::Clamp(X + Offset.X, MapNodeWidth * 0.5f, MapGraphWidth - MapNodeWidth * 0.5f),
		FMath::Clamp(Y + Offset.Y, MapNodeHeight * 0.5f, MapGraphHeight - MapNodeHeight * 0.5f));
}

FVector2D RDFrontendMap::GetMapRoomNodeTopLeft(const FVector2D& Center)
{
	return Center - FVector2D(MapNodeWidth * 0.5f, MapNodeHeight * 0.5f);
}

bool RDFrontendMap::BuildMapConnectionLayout(const FVector2D& FromCenter, const FVector2D& ToCenter, FMapConnectionLayout& OutLayout)
{
	const FVector2D Delta = ToCenter - FromCenter;
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutLayout.mAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	OutLayout.mSize = FVector2D(Length, 4.f);
	OutLayout.mPosition = FromCenter - FVector2D(0.f, 2.f);
	return true;
}

const FMapRoomView* RDFrontendMap::FindMapRoom(const TArray<FMapRoomView>& Rooms, int32 RowIndex, int32 ColumnIndex)
{
	return Rooms.FindByPredicate([RowIndex, ColumnIndex](const FMapRoomView& Room)
	{
		return Room.mRow == RowIndex && Room.mColumn == ColumnIndex;
	});
}
