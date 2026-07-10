/*****************************************************************//**
 * @file   StaticObstacleSpawnData.h
 * @brief  장애물 생성 시 사용되는 정적 Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "StaticObstacleSpawnData.generated.h"

class UBoardActorModel;
class UTacticalAttributeSet;
struct FTacticalAttribute;

/**
 * @brief  장애물 생성 시 사용되는 정적 Data Asset
 */
UCLASS()
class P_RD_API UStaticObstacleSpawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(ObstaclePrimaryAssetTypes::GetObstacleType(), GetFName());
    }

public:
    void PostLoad() override;

#if WITH_EDITOR
public:
    void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

public:
    FName GetKeyName() const;
    float GetDefaultAttributeValue(UWorld* World, TSubclassOf<UTacticalAttributeSet> AttributeSetClass, const FTacticalAttribute& Attrubute, int32 Level) const;

public:
    UPROPERTY(Category = "Spawn", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ModelClass", AssetBundles = "Actor"))
    TSoftClassPtr<UBoardActorModel> mModelClass;
    UPROPERTY(Category = "Spawn", VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "UnitClass", MustImplement = "/Script/P_RD.ActorView",  AssetBundles = "Actor"))
    TSoftClassPtr<AActor> mViewClass;

public:
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Portrait", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mPortrait;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DisplayName"))
    FText mDisplayName;
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description", MultiLine = true))
    FText mDescription;
};
