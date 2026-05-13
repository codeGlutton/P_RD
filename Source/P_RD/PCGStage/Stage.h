/*****************************************************************//**
 * @file   Stage.h
 * @brief  스테이지 객체 구현 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PCGStage/Room.h"

#include "Stage.generated.h"

/**
 * @brief  방 열 객체
 */
USTRUCT(BlueprintType)
struct FStageRow
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
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
	UPROPERTY(SaveGame)
	int32 mStartColumn = 0;
	UPROPERTY(SaveGame)
	TArray<FStageRow> mRoomRows;
};
