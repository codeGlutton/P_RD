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
#include "StaticPassiveData.generated.h"

class UTacticalPassive;
class UTacticalEffect;

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
    FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(SkillPrimaryAssetTypes::GetPassiveType(), GetFName());
    }

public:
    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description"))
    FText mDescription;

    UPROPERTY(Category = "Default", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;

    /**
    * @brief 패시브가 반응하는 시점들 (발동/해제 등 다중)
    *
    * @details
    * 타격 전, 후
    * 피격 전, 후
    * 스킬 시전 전, 후
    * 모션 전, 후
    * 기타 등등
    * 발동 시점과 해제 시점이 다를 수 있어 여러 개를 담는다.
    */
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Timing Tags"))
	FGameplayTagContainer mTimingTags;     // 시점들

    /**
    * @brief 패시브 발동 조건
    *
    * @details
    * 패시브의 발동 조건(mConditionTag),
    * 패시브의 발동 조건 수치(mThresholdValue)
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Passive Trigger Condition"))
    TArray<FPassiveCondition> mPassiveTriggerCondition;

    /**
    * @brief 패시브 클래스
    *
    * @details
    * 고정형은 제네릭(UTacticalPassive_AddStat), 계산형은 Nth 등.
    * 컴포넌트가 이 클래스로 런타임 패시브를 생성하고 본 데이터를 주입한다.
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "PassiveClass", AssetBundles = "Actor"))
    TSoftClassPtr<UTacticalPassive> mPassiveClass;

    /**
    * @brief 적용할 이펙트 "종류" 클래스
    *
    * @details
    * 속성·연산(op)·지속정책은 이 이펙트 클래스가 정의. 양(magnitude)은 mMagnitude로 공급.
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectClass", AssetBundles = "Actor"))
    TSoftClassPtr<UTacticalEffect> mEffectClass;

    /**
    * @brief 적용 수치(양)
    *
    * @details
    * '+5' vs '+10'은 서로 다른 DA. 등급/레벨 스케일이 필요해지면 커브로 확장.
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Magnitude"))
    float mMagnitude = 0.f;

    /**
    * @brief 계산형 패시브의 파라미터(예: 매 N회 발동). 고정형은 미사용.
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Threshold"))
    int32 mThreshold = 0;

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
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StaticPassiveEffect", AssetBundles = "Actor"))
    TSoftObjectPtr<UStaticSkillEffect_Base> mStaticPassiveEffect;

#pragma endregion

};
