/*****************************************************************//**
 * @file   Stage.h
 * @brief  스테이지 객체 구현 헤더
 * @author 모호재
 * @date   2026-05-08
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Stage.generated.h"

struct FRoom;

/**
 * @brief  방 객체
 */
USTRUCT()
struct FStageColumn
{
	GENERATED_BODY()

public:
	TArray<TSharedPtr<FRoom>> mRooms;
};

/**
 * @brief  스테이지 객체
 */
USTRUCT()
struct FStage
{
	GENERATED_BODY()

protected:
	TArray<FStageColumn> mRoomColumns;
};
