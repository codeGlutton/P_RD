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
    void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Class", AssetBundles = BUNDLE_ACTOR))
	TSoftClassPtr<AUnit> mClass;

public:
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillDatas", AssetBundles = BUNDLE_PAD))
	TArray<TSoftObjectPtr<UStaticSkillData>> mSkillDatas;
    UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "EquipmentDatas", AssetBundles = BUNDLE_PAD))
	TArray<TSoftObjectPtr<UStaticEquipmentData>> mEquipmentDatas;

public:
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = BUNDLE_UI))
    TSoftObjectPtr<UTexture2D> mIcon;
    UPROPERTY(Category = "UI", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "Name"))
    FText mName;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description", MultiLine = true))
    FText mDescription;
};
