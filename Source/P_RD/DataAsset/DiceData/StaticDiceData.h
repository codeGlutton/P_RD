/*****************************************************************//**
 * @file   StaticDiceData.h
 * @brief  주사위 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "StaticDiceData.generated.h"

/**
 * @brief  주사위 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticDiceData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(DicePrimaryAssetTypes::GetDiceType(mRarityType), GetFName());
    }

public:
    /**
     * @brief 주사위 희귀도
     * @details
     * Primary Asset을 주사위 희귀도로 분류해두었기 때문에, 해당 값은 Primary Asset Type에 영향을 줌
     */
    UPROPERTY(Category = "Dice", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RarityType"))
    ERarityType mRarityType;
};
