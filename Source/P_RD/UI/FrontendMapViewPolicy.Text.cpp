#include "UI/FrontendMapViewPolicy.h"

FText RDFrontendMap::FrontendMapText(const TCHAR* Key)
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

FText RDFrontendMap::GetMapRoomBadgeText(const FMapRoomView& Room)
{
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

FText RDFrontendMap::GetMapRoomNodeLabel(const FMapRoomView& Room)
{
	if (Room.mIsStartPoint)
	{
		return FrontendMapText(TEXT("MapStartNodeLabel"));
	}

	return FText::Format(
		FrontendMapText(TEXT("MapRoomCompactNodeLabelFormat")),
		FText::AsNumber(Room.mRow + 1),
		FText::AsNumber(Room.mColumn + 1));
}

FText RDFrontendMap::GetMapRoomStateText(const FMapRoomView& Room)
{
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

FText RDFrontendMap::GetRoomDebugTypeText(ERoomType RoomType)
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

FText RDFrontendMap::GetMapRoomDebugNodeLabel(const FMapRoomView& Room)
{
	return FText::Format(
		FrontendMapText(TEXT("MapRoomDebugNodeLabelFormat")),
		FText::AsNumber(Room.mRow + 1),
		FText::AsNumber(Room.mColumn + 1),
		GetRoomDebugTypeText(Room.mType));
}
