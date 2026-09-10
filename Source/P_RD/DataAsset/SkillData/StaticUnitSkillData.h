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

public:
    FText MakeDescription() const override;

#if WITH_EDITOR
public:
    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    /* 스킬 자체 정보 */
public:
    // @brief 사용 타입
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "JobType"))
    EUnitJobType mJobType = EUnitJobType::None;

    // @brief 스킬 타입
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "SkillType"))
    ESkillType mSkillType = ESkillType::Attack;

    // @brief 스킬 희귀도
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, AssetRegistrySearchable, meta = (DisplayName = "RarityType"))
    ERarityType mRarityType = ERarityType::Common;

    // @brief 구매 시 가격
    UPROPERTY(Category = "Skill", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Price"))
    int32 mPrice = 0;

    /* 논리적 설정값들 */
public:
    // @brief 필요 행동력
    UPROPERTY(Category = "BaseLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RequiredActionPoint"))
    int32 mRequiredActionPoint = 0;

public:
    // @brief 컨디션을 무시하고 순수 랜덤성 여부 (해당 스킬은 프리뷰를 제공하지 않음)
    UPROPERTY(Category = "EffectLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IgnoreCondition"))
    bool mIgnoreCondition = false;
};

