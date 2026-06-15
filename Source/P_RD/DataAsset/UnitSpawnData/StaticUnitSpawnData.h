/*****************************************************************//**
 * @file   StaticUnitSpawnData.h
 * @brief  유닛 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "StaticUnitSpawnData.generated.h"

class AUnit;
class UStaticSkillData;
class UStaticEquipmentData;

/**
 * @brief  유닛 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticUnitSpawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
    FName GetKeyName() const;

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Class", AssetBundles = "Actor"))
	TSoftClassPtr<AUnit> mClass;

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticSkillData>> mSkillDatas;
    UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EquipmentDatas", AssetBundles = "PAD"))
	TArray<TSoftObjectPtr<UStaticEquipmentData>> mEquipmentDatas;

public:
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DisplayName"))
    FText mDisplayName;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description", MultiLine = true))
    FText mDescription;
};
