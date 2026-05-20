#include "PCGStage/Stage.h"

FRoom& FStage::GetRoom(int32 RowIndex, int32 ColumnIndex)
{
	return mRoomRows[RowIndex].mRooms[ColumnIndex].GetMutable<FRoom>();
}

const FRoom& FStage::GetRoom(int32 RowIndex, int32 ColumnIndex) const
{
	return mRoomRows[RowIndex].mRooms[ColumnIndex].Get<FRoom>();
}

FRoom& FStage::GetStartRoom()
{
	return mRoomRows[0].mRooms[mStartColumn].GetMutable<FRoom>();
}

const FRoom& FStage::GetStartRoom() const
{
	return mRoomRows[0].mRooms[mStartColumn].Get<FRoom>();
}

FRoom& FStage::GetCurrentRoom()
{
	return mRoomRows[mCurRow].mRooms[mCurColumn].GetMutable<FRoom>();
}

const FRoom& FStage::GetCurrentRoom() const
{
	return mRoomRows[mCurRow].mRooms[mCurColumn].Get<FRoom>();
}
