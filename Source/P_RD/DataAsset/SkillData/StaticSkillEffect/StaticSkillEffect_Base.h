/*****************************************************************//**
 * @file   StaticSkillEffect_Base.h
 * @brief  효과 기본 베이스
 * @author 김준형
 * @date   2026-06-18
 *********************************************************************/
#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "UObject/Object.h"
#include "TAS/Effect/TacticalEffect.h"
#include "StaticSkillEffect_Base.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class P_RD_API UStaticSkillEffect_Base : public UObject
{
	GENERATED_BODY()
	
public:
    /**
    * @brief 효과
    *
    * @details
    * GameplayEffect_Base를 상속받은 Blueprint Class를 참조하는 SoftClassPtr
    *
    * @note
    * Damage, Heal 기타 등등
    */
    UPROPERTY(Category = "SkillEffect", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "GameplayTag" ))
    FGameplayTag mEffectTag;

    UPROPERTY(Category = "SkillEffect", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TacticalEffect"))
    TSubclassOf<UTacticalEffect> mTacticalEffect;

public:
    /**
    * @brief 캐스터의 스킬 포인트를 (AS가 존재하지 않으면 기본값을) 기반으로 FBoardCombatTargetSnapshotData를 만들어줍니다.
    *
    * @details
    * 효과값을 반영하여 기본적인 효과 데이터를 만들어줍니다.
    * 실패 시 false를 반환합니다.
    */
    virtual bool CreateBaseEffectContainer(TWeakObjectPtr<class UBoardActorModel> CasterActor, OUT struct FBoardCombatTargetSnapshotData& Container) PURE_VIRTUAL(UStaticSkillEffect_Base::CreateBaseEffectContainer, return false;)
};


