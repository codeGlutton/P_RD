/*****************************************************************//**
 * @file   UStaticPassiveData.h
 * @brief  패시브 정적 데이터
 * @author 김준형, 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "StaticPassiveData.generated.h"

class UTacticalPassive;
class UTacticalEffect;

/**
 * @brief 수량 조건 - 발동 조건을 통과한 타겟이 얼마나 있어야 하는가
 */
UENUM(BlueprintType)
enum class EPassiveTargetQuantifier : uint8
{
	Any		UMETA(DisplayName = "Any"),		// 타겟이 하나라도 있으면 발동
	All		UMETA(DisplayName = "All"),		// 모든 타겟이 자격을 갖춰야 발동
};

USTRUCT(BlueprintType)
struct FPassiveCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag mConditionTag;     // 조건 태그

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float mThresholdValue = 0.f;    // 기준 수치 (예: 데미지 10, HP 30%)
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
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description"))
    FText mDescription;

    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;

public:
    /**
    * @brief 패시브 발동 시점 (단일)
    *
    * @details
    * 이펙트를 적용(발동)할 시점(예: 공격 시작, 타격 전 등).
    * 주기형 버프는 여기에 시작 시점을, mDeactivateTimingTag에 끝 시점을 둔다.
    */
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Activate Timing"))
	FGameplayTag mActivateTimingTag;

    /**
    * @brief 패시브 해제 시점 (단일)
    *
    * @details
    * 적용 중인 이펙트를 제거(해제)할 시점(예: 공격 끝).
    */
	UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Deactivate Timing"))
	FGameplayTag mDeactivateTimingTag;

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
    * @brief 수량 조건
    *
    * @details
    * 발동 조건을 통과한 타겟이 얼마나 있어야 발동하는지 여부.
    * Any=하나라도, All=전부.
    */
    UPROPERTY(Category = "Passive", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Target Quantifier"))
    EPassiveTargetQuantifier mTargetQuantifier = EPassiveTargetQuantifier::Any;

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
};
