// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataAsset/SkillData/StaticSkillEffect/StaticSkillEffect_Base.h"
#include "StaticSkillEffect_Damage.generated.h"

/**
 * 
 */
UCLASS()
class UStaticSkillEffect_Damage : public UStaticSkillEffect_Base
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

public:
    virtual bool ApplyOtherPointToSkillPoint(
        float SkillPoint,
        TWeakObjectPtr<class UBoardActorModel> SourceActor,
        OUT struct FActiveTacticalEffectHandle& EffectHandle) override;

    virtual void ApplySkillEffect(
        TWeakObjectPtr<class UBoardActorModel> SourceActor,
        TWeakObjectPtr<class UBoardActorModel> TargetActor,
        struct FBoardCombatTargetSnapshotData* SourceSnapShot = nullptr,
        struct FBoardCombatTargetSnapshotData* TargetSnapShot = nullptr) override;

};