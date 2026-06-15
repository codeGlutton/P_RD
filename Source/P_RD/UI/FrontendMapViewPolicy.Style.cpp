#include "UI/FrontendMapViewPolicy.h"

FLinearColor RDFrontendMap::GetMapRoomTypeColor(ERoomType RoomType)
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

FLinearColor RDFrontendMap::GetMapRoomPanelColor(const FMapRoomView& Room)
{
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

FLinearColor RDFrontendMap::GetMapConnectionLineColor(const FMapRoomView& FromRoom)
{
	return FromRoom.mState != EMapRoomState::Locked
		? FLinearColor(0.445f, 0.760f, 0.780f, 0.92f)
		: FLinearColor(0.250f, 0.300f, 0.320f, 0.58f);
}

FSlateColor RDFrontendMap::GetMapRoomTextColor(const FMapRoomView& Room)
{
	return Room.mState == EMapRoomState::Locked ? LockedColor : TitleColor;
}
