/*****************************************************************//**
 * @file   StaticFrontendRoomSpawnData.h
 * @brief  타이틀 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "StaticFrontendRoomSpawnData.generated.h"

class UStaticPlayerUnitSpawnData;

/**
 * @brief  타이틀 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticFrontendRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetFrontendRoomType(), GetFName());
	}

public:
	UPROPERTY(Category = "PlayerUnit", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectablePlayerUnits", AssetBundles = BUNDLE_PAD))
	TArray<TSoftObjectPtr<UStaticPlayerUnitSpawnData>> mPlayableUnits;
};
