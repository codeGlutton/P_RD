// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Base.h"
#include "StaticSkillEffect_Stat.generated.h"

/**
 * 
 */
UCLASS()
class UStaticSkillEffect_Stat : public UStaticSkillEffect_Base
{
	GENERATED_BODY()

public:

    /**
    * @brief 선택 대상
    *
    * @details
    * (Owner: 소유자, Instigator: 유발자, Selected_Tile: 선택된 대상)
    * 어떤 것은 타일을, 어떤 것은 유닛을 이렇게 하니까 너무 복잡하게 되어 포기
    */
    //UPROPERTY(Category = "SkillEffectLayer", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetScope"))
    //ETargetScope mTargetScope;

    /**
    * @brief 제외 대상
    *
    * @details
    * 없음, 자신, 아군, 적
    */
    UPROPERTY(Category = "SkillEffectStat", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETargetFilter", DisplayName = "TargetFilter"))
    uint8 mTargetFilter = 3;

    /**
    * @brief 효과 기본 값
    * @details
    * 결과값 = Default + 눈금 * Ratio
    */
    UPROPERTY(Category = "SkillEffectStat", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultValue"))
    int32 mEffectDefaultValue;

    /**
    * @brief 효과 비율 값
    * @details
    * 결과값 = Default + 눈금 * Ratio
    */
    UPROPERTY(Category = "SkillEffectStat", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RatioValue"))
    float mEffectRatioValue;
};