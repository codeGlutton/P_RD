#include "PCGStage/Stage.h"

FRoom& FStage::GetStartRoom()
{
	return mRoomRows[0].mRooms[mStartColumn].GetMutable<FRoom>();
}

const FRoom& FStage::GetStartRoom() const
{
	return mRoomRows[0].mRooms[mStartColumn].Get<FRoom>();
}
