/*****************************************************************//**
 * @file   StaticUnitSkillData.h
 * @brief  유닛 스킬 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-08-01
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "StaticUnitSkillData.generated.h"
/**
 * @brief  스킬 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticUnitSkillData : public UStaticSkillData
{
	GENERATED_BODY()

    /* UStaticSkillData 상속 */
public:
    FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(SkillPrimaryAssetTypes::GetActiveType(), GetFName());
    }

#if WITH_EDITOR
public:
    FText MakeDescription() const override;
    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    /* 스킬 자체 정보 */
public:
    // @brief 사용 타입
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "JobType"))
    EPlayerJobType mJobType = EPlayerJobType::None;

    // @brief 스킬 타입
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "SkillType"))
    ESkillType mSkillType;

    // @brief 스킬 희귀도
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "RarityType"))
    ERarityType mRarityType;

    // @brief 구매 시 가격
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Price"))
    int32 mPrice;

    /* 논리적 설정값들 */
public:
    // @brief 필요 행동력
    UPROPERTY(Category = "BaseLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RequiredActionPoint"))
    int32 mRequiredActionPoint;
};

