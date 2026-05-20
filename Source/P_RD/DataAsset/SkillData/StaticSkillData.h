/*****************************************************************//**
 * @file   StaticSkillData.h
 * @brief  스킬 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "StaticSkillData.generated.h"

/**
 * @brief  스킬 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticSkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    /**
     * @brief 스킬 희귀도
     * @details
     * Primary Asset을 스킬 타입과 희귀도로 분류해두었기 때문에, 해당 값은 Primary Asset Type에 영향을 줌
     */
    UPROPERTY(Category = "Rarity", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RarityType"))
    ERarityType mRarityType;

    /* 필요하다고 생각하는 것들 */

    //UPROPERTY(Category = "Ability", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Ability", BUNDLE_ACTOR))
    //TSoftClassPtr<UGameplayAbility> mAbility;
    //
    //UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", BUNDLE_UI))
    //TSoftObjectPtr<UTexture2D> mIcon;
    //
    //UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Name"))
    //FText mName;
    // 
    //UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description"))
    //FText mDescription;
};
