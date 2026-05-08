/*****************************************************************//**
 * @file   StaticShopRoomSpawnData.h
 * @brief  상점 방 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/StaticRoomSpawnData.h"
#include "StaticShopRoomSpawnData.generated.h"

/**
 * @brief  상점 방 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticShopRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		const FString TypeString = FString(TEXT("ShopRoom")) + FString::FromInt(mStageLevel);
		return FPrimaryAssetId(*TypeString, GetFName());
	}

public:
	UPROPERTY(Category = "Unit", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ShopOwnerSpawnData", AssetBundles = "ShopOwnerSpawnData"))
	FActorSpawnData mShopOwnerSpawnData;
};
