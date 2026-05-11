/*****************************************************************//**
 * @file   Room.h
 * @brief  방 데이터 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "Room.generated.h"

UENUM(BlueprintType)
enum class ERoomType : uint8
{
	None = 0 UMETA(Hidden),
	Treasure,
	Shop,
	Monster,
	Elite,
	Boss,
};

/**
 * @brief  방 데이터
 */
USTRUCT()
struct FRoom
{
	GENERATED_BODY()

public:
	ERoomType mType = ERoomType::None;
	int32 mRow = 0;
	int32 mColumn = 0;
	TArray<TSharedPtr<FRoom>> mNextRooms;
	bool mIsSelected = false;

public:
	FVector2D mRenderPosOffset;

public:
	FPrimaryAssetId mStaticSpawnDataId;
};
