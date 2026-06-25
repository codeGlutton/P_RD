// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Base.h"
#include "StaticSkillEffect_Move_Force.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API UStaticSkillEffect_Move_Force : public UStaticSkillEffect_Base
{
    GENERATED_BODY()

public:

    /**
    * @brief 제외 대상
    *
    * @details
    * 없음, 자신, 아군, 적
    */
    UPROPERTY(Category = "SkillEffectMoveForce", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETargetFilter", DisplayName = "TargetFilter"))
    uint8 mTargetFilter = (uint8)ETargetFilter::All;

    /**
    * @brief 효과 기본 값
    * @details
    * 결과값 = Default + 눈금 * Ratio
    */
    UPROPERTY(Category = "SkillEffectMoveForce", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "DefaultDistance"))
    int32 mEffectDefaultDistance;

    /**
    * @brief 효과 비율 값
    * @details
    * 결과값 = Default + 눈금 * Ratio
    */
    UPROPERTY(Category = "SkillEffectMoveForce", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "RatioDistance"))
    float mEffectRatioDistance;

public:
    virtual bool CreateBaseEffectContainer(TWeakObjectPtr<class UBoardActorModel> CasterActor, OUT struct FBoardCombatTargetSnapshotData& Container) override;
};
