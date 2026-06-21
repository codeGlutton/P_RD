/*****************************************************************//**
 * @file   UStaticPassiveData.h
 * @brief  패시브 정적 데이터
 * @author 김준형
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/BundleType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Base.h"
#include "Passive/PassiveLogic/PassiveLogic.h"
#include "Passive/DynamicPassiveData/DynamicPassiveData_Base.h"
#include "StaticPassiveData.generated.h"

USTRUCT(BlueprintType)
struct FPassiveCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag mConditionTag;     // 조건 태그

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float mThresholdValue;          // 기준 수치 (예: 데미지 10, HP 30%)
};

/**
 * 
 */
UCLASS()
class P_RD_API UStaticPassiveData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description"))
    FText mDescription;

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;

    /**
    * @brief 패시브 적용 시점
    *
    * @details
    * 타격 전, 후
    * 피격 전, 후
    * 스킬 시전 전, 후
    * 모션 전, 후
    * 기타 등등
    */
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Passive Trigger Timig"))
	FGameplayTag mPassiveTriggerTimig;     // 시점

    /**
    * @brief 패시브 발동 조건
    *
    * @details
    * 패시브의 발동 조건(mConditionTag),
    * 패시브의 발동 조건 수치(mThresholdValue)
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Passive Trigger Condition"))
    TArray<FPassiveCondition> mPassiveTriggerCondition;

#pragma region Effect

    /**
    * @brief 누구를 중심으로?
    *
    * @details
    * 소유자의 타일을 중심으로?(패시브를 가지고 있는 주인)
    * 유발자의 타일을 중심으로?(패시브와 연관된 대상 : 공격하고 있다면 피격자, 피격되고 있다면 공격자)
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectCenterType"))
    EEffectCenterType mEffectCenterType;

    /*
    * @brief 효과 범위 패턴
    *
    * @details
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectAreaType"))
    EEffectPattern mEffectPattern;

    /**
    * @brief 관통 여부
    *
    * @details
    * true면 유닛 넘어도 영향 대상
    * false면 유닛 넘어는 영향 대상이 아님
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsPenetration"))
    bool mIsPenetration;

    /**
    * @brief 기본 영향 범위
    *
    * @details
    * 고정값
    * 사정거리는 항상 DefaultArea + RatioArea * 눈금
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectDefaultArea"))
    int32 mEffectDefaultArea;

    /**
    * @brief 패시브 효과 정적 데이터
    *
    * @details
    * 패시브 효과
    * 데미지, 힐, 밀치기 등
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StaticPassiveEffect"))
    TObjectPtr<UStaticSkillEffect_Base> mStaticPassiveEffect;

#pragma endregion

    /**
    * @brief 패시브 로직
    *
    * @details
    * 함수만 사용 예정
    */
	UPROPERTY(Category = "PassiveLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Passive Logic"))
	TSubclassOf<UPassiveLogic> mPassiveLogic;


    /**
    * @brief 패시브 동적 데이터
    *
    * @details
    * 패시브 장착 시 생성
    */
    UPROPERTY(Category = "PassiveLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Dynamic Passive Data"))
    TSubclassOf<UDynamicPassiveData_Base> mPassiveDynamicData;

};
