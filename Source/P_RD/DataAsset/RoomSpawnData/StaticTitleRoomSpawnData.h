/*****************************************************************//**
 * @file   StaticTitleRoomSpawnData.h
 * @brief  타이틀 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "StaticTitleRoomSpawnData.generated.h"

/**
 * @brief  타이틀 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticTitleRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetTitleRoomType(), GetFName());
	}
};
