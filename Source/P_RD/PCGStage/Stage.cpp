#include "PCGStage/Stage.h"

void FStage::SetCurrentRoom(int32 RowIndex, int32 ColumnIndex)
{
	checkf(HasRoom(RowIndex, ColumnIndex) == true, TEXT("잘못된 방 인덱스 세팅"));

	mCurRow = RowIndex;
	mCurColumn = ColumnIndex;

	const bool IsEnteredRoom = mRoomRows[RowIndex].mRooms[ColumnIndex].GetMutable<FRoom>().mWasSelected;
	mRoomRows[RowIndex].mRooms[ColumnIndex].GetMutable<FRoom>().mWasSelected = true;

	if (IsEnteredRoom == false)
	{
		// 방 클리어 데이터 초기화
		SetRoomClearData(FRoomClearData());
	}
}

void FStage::SetRoomClearData(const FRoomClearData& ClearData)
{
	mClearData = ClearData;
}

FRoom& FStage::GetRoom(int32 RowIndex, int32 ColumnIndex)
{
	checkf(HasRoom(RowIndex, ColumnIndex) == true, TEXT("잘못된 방 인덱스 접근"));
	return mRoomRows[RowIndex].mRooms[ColumnIndex].GetMutable<FRoom>();
}

const FRoom& FStage::GetRoom(int32 RowIndex, int32 ColumnIndex) const
{
	checkf(HasRoom(RowIndex, ColumnIndex) == true, TEXT("잘못된 방 인덱스 접근"));
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

bool FStage::HasRoom(int32 RowIndex, int32 ColumnIndex) const
{
	if (mRoomRows.IsValidIndex(RowIndex) == false || mRoomRows[RowIndex].mRooms.IsValidIndex(ColumnIndex) == false)
	{
		return false;
	}

	const TInstancedStruct<FRoom>& TargetRoom = mRoomRows[RowIndex].mRooms[ColumnIndex];
	return TargetRoom.IsValid() && TargetRoom.Get<FRoom>().mType != ERoomType::None;
}
