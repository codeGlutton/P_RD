/*****************************************************************//**
 * @file   StaticEquipmentData.h
 * @brief  장비 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "DataAsset/PassiveData/StaticPassiveData.h"
#include "TAS/Effect/TacticalEffect.h"
#include "StaticEquipmentData.generated.h"

/**
 * @brief  장비 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticEquipmentData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Name"))
    FText mName;

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Price"))
    int32 mPrice;

    /**
     * @brief 장비 희귀도
     * @details
     * Primary Asset을 장비 타입과 희귀도로 분류해두었기 때문에, 해당 값은 Primary Asset Type에 영향을 줌
     */
    UPROPERTY(Category = "Equipment", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RarityType"))
    ERarityType mRarityType;

    UPROPERTY(Category = "Equipment", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Passive", AssetBundles = "Actor"))
    TArray<TSoftObjectPtr<UStaticPassiveData>> mStaticPassiveData;

    /**
     * @brief 장비 고유 스탯 수정자
     *
     * @details
     * 장착 시 무한(Infinite) 이펙트로 AttributeSet에 즉시 적용하고, 해제 시 핸들로 제거.
     */
    UPROPERTY(Category = "Equipment", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StatModifiers"))
    TArray<FTacticalModifierInfo> mStatModifiers;

    /**
     * @brief 장비 슬롯 타입 반환 (서브클래스가 자기 슬롯으로 override)
     */
    virtual EEquipmentType GetEquipmentType() const PURE_VIRTUAL(UStaticEquipmentData::GetEquipmentType, return EEquipmentType::Count;)
};
