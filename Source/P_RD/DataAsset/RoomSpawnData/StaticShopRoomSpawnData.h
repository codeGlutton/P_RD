/*****************************************************************//**
 * @file   StaticShopRoomSpawnData.h
 * @brief  상점 방 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-07
 *********************************************************************/

#pragma once

#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "StaticShopRoomSpawnData.generated.h"

/**
 * @brief  상점 방 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticShopRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

public:
	void PostInitProperties() override;
	void PostLoad() override;

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetShopRoomType(mStageLevel), GetFName());
	}

public:
	/**
	 * @brief 현재 방이 등장할 수 있는 스테이지 레벨
	 * @details
	 * Primary Asset을 방 타입과 레벨 별로 분류해두었기 때문에, 해당 값은 Primary Asset Type에 영향을 줌
	 */
	UPROPERTY(Category = "Stage", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StageLevel"))
	EStageLevelType mStageLevel;

public:
	UPROPERTY(Category = "Unit", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ShopOwnerClass", AssetBundles = BUNDLE_ACTOR))
	TSoftClassPtr<AActor> mShopOwnerClass;
};
