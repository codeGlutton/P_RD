#include "UI/Room/RoomUIModel.h"

/** @brief 방 공통 플레이어 요약을 저장하고 상단바에 변경을 알린다. */
void URoomUIModel::SetPlayerSummary(const FRoomPlayerSummaryUI& Summary)
{
	mPlayerSummary = Summary;
	OnUIChanged.Broadcast();
}
