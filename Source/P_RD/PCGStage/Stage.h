/*****************************************************************//**
 * @file   Stage.h
 * @brief  스테이지 객체 구현 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/Room.h"
#include "DataAsset/StageSpawnData/StageLevelType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Stage.generated.h"

/**
 * @brief  방 열 객체
 */
USTRUCT(BlueprintType)
struct FRoomRow
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Room, SaveGame, VisibleAnywhere, meta = (DisplayName = "Rooms"))
	TArray<TInstancedStruct<FRoom>> mRooms;
};

/**
 * @brief  스테이지 객체
 */
USTRUCT(BlueprintType)
struct FStage
{
	GENERATED_BODY()

public:
	void SetCurrentRoom(int32 RowIndex, int32 ColumnIndex);
	void SetRoomClearData(const FRoomClearData& ClearData);

public:
	FRoom& GetRoom(int32 RowIndex, int32 ColumnIndex);
	const FRoom& GetRoom(int32 RowIndex, int32 ColumnIndex) const;

	FRoom& GetStartRoom();
	const FRoom& GetStartRoom() const;

	FRoom& GetCurrentRoom();
	const FRoom& GetCurrentRoom() const;

	bool HasRoom(int32 RowIndex, int32 ColumnIndex) const;

public:
	UPROPERTY(Category = Stage, SaveGame, VisibleAnywhere, meta = (DisplayName = "StageLevel"))
	EStageLevelType mStageLevel = EStageLevelType::None;
	
	UPROPERTY(Category = Stage, SaveGame, VisibleAnywhere, meta = (DisplayName = "RoomRows"))
	TArray<FRoomRow> mRoomRows;
	UPROPERTY(Category = Stage, SaveGame, VisibleAnywhere, meta = (DisplayName = "StartColumn"))
	int32 mStartColumn = 0;

public:
	UPROPERTY(Category = Play, SaveGame, VisibleAnywhere, meta = (DisplayName = "CurColumn"))
	int32 mCurColumn = INDEX_NONE;
	UPROPERTY(Category = Play, SaveGame, VisibleAnywhere, meta = (DisplayName = "CurRow"))
	int32 mCurRow = INDEX_NONE;

	UPROPERTY(Category = Play, SaveGame, VisibleAnywhere, meta = (DisplayName = "ClearData"))
	FRoomClearData mClearData;

public:
	UPROPERTY(Category = Asset, SaveGame, VisibleAnywhere, meta = (DisplayName = "StaticStageSpawnDataId"))
	FPrimaryAssetId mStaticStageSpawnDataId;
};
