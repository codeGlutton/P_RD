// Fill out your copyright notice in the Description page of Project Settings.

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
    UPROPERTY(Category = "SkillEffectLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "GameplayTag"))
    FGameplayTag mEffectTag;
};


