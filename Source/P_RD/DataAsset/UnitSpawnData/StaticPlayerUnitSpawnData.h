/*****************************************************************//**
 * @file   StaticPlayerUnitSpawnData.h
 * @brief  플레이어 유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "DataAsset/UnitSpawnData/StaticUnitSpawnData.h"
#include "DataAsset/UnitSpawnData/PlayerJobType.h"
#include "StaticPlayerUnitSpawnData.generated.h"

class UStaticDiceData;

/**
 * @brief  플레이어 유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticPlayerUnitSpawnData : public UStaticUnitSpawnData
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(UnitPrimaryAssetTypes::GetPlayerUnitType(), GetFName());
	}

#if WITH_EDITOR
public:
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	float GetDefaultMaxHP(int32 Difficulty) const;
	float GetDefaultMoney(int32 Difficulty) const;

public:
	UPROPERTY(Category = "Spawn", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "JobType"))
	EPlayerJobType mJobType;

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceDatas", AssetBundles = BUNDLE_PAD))
	TArray<TSoftObjectPtr<UStaticDiceData>> mDiceDatas;

public:
	UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Portrait", AssetBundles = BUNDLE_UI))
	TSoftObjectPtr<UTexture2D> mPortrait;
};
