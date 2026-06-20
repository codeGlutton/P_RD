/*****************************************************************//**
 * @file   StaticSkillEffect_Base.h
 * @brief  효과 기본 베이스
 * @author 김준형
 * @date   2026-06-18
 *********************************************************************/
#pragma once

#include "GAS/GASMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "UObject/Object.h"
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
    TSoftObjectPtr<class UTacticalEffect> mTacticalEffect;

public:
    virtual class UTacticalEffectContext* CreateContext(TWeakObjectPtr<class UBoardActorModel> CasterActor) PURE_VIRTUAL(UStaticSkillEffect_Base::CreateContext, return nullptr;)
};


