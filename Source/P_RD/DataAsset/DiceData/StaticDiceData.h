/*****************************************************************//**
 * @file   StaticDiceData.h
 * @brief  주사위 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "StaticDiceData.generated.h"

class UTexture2D;

/** @brief 주사위 한 면의 정적 표시/게임플레이 값입니다. */
// 면 배열 index가 굴러진 물리 면이다. mValue는 전투 값, mTexture는 해당 면에 덮을 선택 텍스처다.
USTRUCT(BlueprintType)
struct P_RD_API FStaticDiceFaceData
{
	GENERATED_BODY()

	UPROPERTY(Category = "Dice", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Value"))
	int32 mValue = 1;

	UPROPERTY(Category = "Dice", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Texture", AssetBundles = "UI"))
	TSoftObjectPtr<UTexture2D> mTexture;
};

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

	/** @brief 주사위 면 수입니다. 지원 비주얼은 d2/d4/d6/d8/d12/d20입니다. */
	UPROPERTY(Category = "Dice", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "FaceCount", ClampMin = "2", UIMin = "2"))
	int32 mFaceCount = 6;

	/** @brief 면별 값/텍스처입니다. 부족한 면은 1..FaceCount 기본값으로 채우고, 초과분은 무시합니다. */
	UPROPERTY(Category = "Dice", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Faces"))
	TArray<FStaticDiceFaceData> mFaces;
};
