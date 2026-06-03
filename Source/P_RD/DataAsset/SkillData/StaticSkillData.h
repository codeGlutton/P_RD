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
#include "../../SRPGFramework/SRPGFrameworkType.h"
#include "SkillType.h"
#include "StaticSkillData.generated.h"

 /**
 * @brief 필터 유형
 * @details
 * 추후 위치 변경 예정
 */
UENUM(BlueprintType)
enum class EEffectFilter : uint8
{
    Caster = 0,
    Others,
    All,
    Count		UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FGameplayEffectData
{
    GENERATED_BODY()

    /**
    * @brief 필터 유형
    * @details
    * (Caster: 시전자만, All: 영향 범위 전체, Others: 시전자 제외 모든 타일)
    */
    UPROPERTY(Category = "GameplayEffectData", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectTarget"))
    EEffectFilter mGameplayEffectFilter;

    /**
    * @brief 효과
    * @details
	* GameplayEffect_Base를 상속받은 Blueprint Class를 참조하는 SoftClassPtr
    */
    UPROPERTY(Category = "GameplayEffectData", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "GameplayEffect"))
    TSoftClassPtr<class UGameplayEffect_Base> mGameplayEffect;

    /**
    * @brief 효과 기본 값
    * @details
    * 결과값 = Default + 눈금 * Ratio
    */
    UPROPERTY(Category = "GameplayEffectData", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultValue"))
    int32 mGameplayEffectDefaultValue;

    /**
    * @brief 효과 비율 값
    * @details
    * 결과값 = Default + 눈금 * Ratio
    */
    UPROPERTY(Category = "GameplayEffectData", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RatioValue"))
    float mGameplayEffectRatioValue;
};

USTRUCT(BlueprintType)
struct FSkillAction
{
    GENERATED_BODY()

    /**
    * @brief 보여줄 애니메이션
    * @details
    * (ex: Anim.Attack, Anim.Spell)
    */
    UPROPERTY(Category = "SkillAction", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Animation"))
    FGameplayTag mAnimationTag;

    /**
    * @brief 효과 레이어
    * @details
    * 
    */
    UPROPERTY(Category = "SkillAction", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "GameplayEffectDatas"))
    TArray<FGameplayEffectData> mGameplayEffectDatas;
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
    */
    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillType"))
    ESkillType mSkillType;
    
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

#pragma region AimRange

    /* 조준 범위 */

    /**
    * @brief 조준 범위 유형
    * @details
    */
    UPROPERTY(Category = "AimRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AimType"))
    EAimPattern mAimPattern;

    /**
    * @brief 곡사 여부
    * @details 
    * true면 적 넘어도 선택 가능
    * false면 적 넘어는 선택 불가
    */
    UPROPERTY(Category = "AimRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsIndirect"))
    bool mIsIndirect;

    /**
    * @brief 장애물(유닛) 조준 가능 여부
    * @details 
    * true면 장애물(유닛)이 있는 타일 선택 가능
    * false면 장애물(유닛)이 있는 타일 선택 불가
    */
    UPROPERTY(Category = "AimRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CanAimObstacle"))
    bool mCanAimObstacle = true;

    /**
    * @brief 기본 조준 거리
    * @details 
    * 고정값
    */
    UPROPERTY(Category = "AimRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AimDefaultRange"))
    int32 mAimDefaultRange;

    /**
    * @brief 비율 조준 거리
    * @details
    * 주사위값에 따라 변하는 조준 범위 
    * 사정거리는 항상 DefaultRange + RatioRange
    */
    UPROPERTY(Category = "AimRange", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AimRatioRange"))
    float mAimRatioRange;

#pragma endregion AimRange

#pragma region EffectArea

    /* 영향 범위 */

    /**
    * @brief 영향 범위 유형
    * @details
    */
    UPROPERTY(Category = "EffectArea", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectAreaType"))
    EEffectPattern mEffectPattern;

    /**
    * @brief 관통 여부
    * @details
    * true면 유닛 넘어도 영향 대상
    * false면 유닛 넘어는 영향 대상이 아님
    */
    UPROPERTY(Category = "EffectArea", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsPenetration"))
    bool mIsPenetration;

    /**
    * @brief 기본 영향 범위
    * @details
    * 고정값
    */
    UPROPERTY(Category = "EffectArea", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectDefaultArea"))
    int32 mEffectDefaultArea;

    /**
    * @brief 비율 영향 범위
    * @details
    * 주사위값에 따라 변하는 영향 범위 비율
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
    UPROPERTY(Category = "SkillAction", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillActions"))
    TArray<FSkillAction> mSkillActions;

};
