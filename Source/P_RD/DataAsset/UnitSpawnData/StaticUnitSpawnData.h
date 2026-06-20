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

class UObjectModel;
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
    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
    FName GetKeyName() const;

public:
    UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ModelClass", AssetBundles = "Actor"))
    TSoftClassPtr<UObjectModel> mModelClass;
	// [핫픽스] 모델-뷰 매핑이 아직 비어 있어 mModelClass 파생만으로는 항상 null이 된다(유닛 View 미구현).
	// 매핑/유닛 뷰가 갖춰질 때까지 직접 지정도 허용한다(EditAnywhere). 매핑이 채워지면 다시 파생으로 좁혀도 됨.
	UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UnitClass", AssetBundles = "Actor"))
	TSoftClassPtr<AUnit> mUnitClass;

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
