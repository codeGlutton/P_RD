/*****************************************************************//**
 * @file   StaticTreasureRoomSpawnData.h
 * @brief  보상 방 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-12
 *********************************************************************/

#pragma once

#include "DataAsset/RoomSpawnData/StaticRoomSpawnData.h"
#include "StaticTreasureRoomSpawnData.generated.h"

/**
 * @brief  보상 방 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticTreasureRoomSpawnData : public UStaticRoomSpawnData
{
	GENERATED_BODY()

public:
	void PostInitProperties() override;

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(RoomPrimaryAssetTypes::GetTreasureRoomType(mStageLevel), GetFName());
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
	UPROPERTY(Category = "Actor", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TreasureBoxClass", AssetBundles = "Actor"))
	TSoftClassPtr<AActor> mTreasureBoxClass;
};
