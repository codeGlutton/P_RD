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

USTRUCT(BlueprintType)
struct FEffectLayer
{
    GENERATED_BODY()

    /**
    * @brief 필터 유형
    * @details
    * 추후 enum 또는 태그로 변경할 예정
    * (Caster: 시전자, Target: 타겟 타일 전체, Others: 시전자 제외 모든 타일)
    */
    UPROPERTY(Category = "EffectLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTarget"))
    int32 mEffectTarget;

    /**
    * @brief 효과 태그
    * @details
    * 추후 GameplayTag로 변경할 예정
    * (ex: Effect.Damage.Fire, Effect.Buff.Shield)
    */
    UPROPERTY(Category = "EffectLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTag"))
    FString mEffectTag;

    /**
    * @brief 효과 기본 값
    * @details
    * 결과값 = DefaultValue + 눈금 * Ratio
    */
    UPROPERTY(Category = "EffectLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultValue"))
    int32 mDefaultValue;

    /**
    * @brief 효과 비율 값
    * @details
    * 결과값 = DefaultValue + 눈금 * Ratio
    */
    UPROPERTY(Category = "EffectLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RatioValue"))
    int32 mRatioValue;
};

USTRUCT(BlueprintType)
struct FSkillLayer
{
    GENERATED_BODY()

    /**
    * @brief 보여줄 애니메이션
    * @details
    * 추후 enum 또는 태그로 변경할 예정
    * (ex: Anim.Attack, Anim.Spell)
    */
    UPROPERTY(Category = "SkillLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Animation"))
    int32 mAnimationTag;

    /**
    * @brief 효과 레이어
    * @details
    * 
    */
    UPROPERTY(Category = "SkillLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectLayers"))
    TArray<FEffectLayer> mEffects;
};



/**
 * @brief  스킬 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS(abstract)
class P_RD_API UStaticSkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:


#pragma region Default

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Name"))
    FText mName;

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description"))
    FText mDescription;

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", BUNDLE_UI))
    TSoftObjectPtr<UTexture2D> mIcon;

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Price"))
    int32 mPrice;

    /**
    * @brief 스킬 유형
    * @details
    * 공격, 주문
    * 추후 enum으로 바꿀 것
    */
    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillType"))
    int32 mSkillType;
    
    /**
    * @brief 스킬 희귀도
    * @details
    * Primary Asset을 스킬 타입과 희귀도로 분류해두었기 때문에, 해당 값은 Primary Asset Type에 영향을 줌
    */
    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RarityType"))
    ERarityType mRarityType;

    /**
    * @brief 필요 주사위
    */
    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DiceCount"))
    int32 mDiceCount;


#pragma endregion Default

#pragma region SelectRange

    /* 사정거리 */

    /**
    * @brief 사정 거리 유형
    * @details
    * 추후 enum으로 변경 예정
    */
    UPROPERTY(Category = "SelectRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectType"))
    int32 mSelectType;

    /**
    * @brief 곡사 여부
    * @details 
    * true면 적 넘어도 선택 가능
    * false면 적 넘어는 선택 불가
    */
    UPROPERTY(Category = "SelectRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsIndirect"))
    bool mIsIndirect;

    /**
    * @brief 장애물(유닛) 선택 가능 여부
    * @details 
    * true면 장애물(유닛)이 있는 타일 선택 가능
    * false면 장애물(유닛)이 있는 타일 선택 불가
    */
    UPROPERTY(Category = "SelectRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CanSelectObstacle"))
    bool mCanSelectObstacle;

    /**
    * @brief 기본 사정 거리
    * @details 
    * 고정값
    */
    UPROPERTY(Category = "SelectRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectDefaultRange"))
    int32 mSelectDefaultRange;

    /**
    * @brief 비율 사정 거리
    * @details
    * 주사위값에 따라 변하는 사정거리양
    * 사정거리는 항상 DefaultRange + RatioRange
    */
    UPROPERTY(Category = "SelectRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SelectRatioRange"))
    float mSelectRatioRange;

#pragma endregion SelectRange

#pragma region EffectArea

    /* 타격 범위 */

    /**
    * @brief 효과 범위 유형
    * @details
    * 추후 enum으로 변경 예정
    */
    UPROPERTY(Category = "EffectArea", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectAreaType"))
    int32 mEffectAreaType;

    /**
    * @brief 관통 여부
    * @details
    * true면 적 넘어도 효과 대상
    * false면 적 넘어는 효과 대상이 아님
    */
    UPROPERTY(Category = "EffectArea", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsPenetration"))
    bool mIsPenetration;

    /**
    * @brief 기본 효과 범위
    * @details
    * 고정값
    */
    UPROPERTY(Category = "EffectArea", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectDefaultArea"))
    int32 mEffectDefaultArea;

    /**
    * @brief 비율 효과 범위
    * @details
    * 주사위값에 따라 변하는 효과 범위양
    * 사정거리는 항상 DefaultArea + RatioArea
    */
    UPROPERTY(Category = "EffectArea", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectRatioArea"))
    float mEffectRatioArea;

#pragma endregion EffectArea

    /**
    * @brief 스킬 효과
    * @details
    * 
    */
    UPROPERTY(Category = "Effect", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillLayers"))
    TArray<FSkillLayer> mSkillLayers;

};
