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
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Actor/TileMap/TileLayer.h"
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
    //UPROPERTY(Category = "SkillEffect", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "GameplayTag" ))
    //FGameplayTag mEffectTag;
    //
    //UPROPERTY(Category = "SkillEffect", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TacticalEffect"))
    //TSubclassOf<UTacticalEffect> mTacticalEffect;


    UPROPERTY(Category = "SkillEffectStat", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag", DisplayName = "TargetLayer"))
    uint8 mTileLayer = (uint8)ETileLayerFlag::Unit;

    /**
    * @brief 제외 대상
    *
    * @details
    * 없음, 자신, 아군, 적
    */
    UPROPERTY(Category = "SkillEffectStat", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETargetFilter", DisplayName = "TargetFilter"))
    uint8 mTargetFilter = (uint8)ETargetFilter::All;

public:
    /**
    * @brief SkillPoint를 특정 포인트로 변경합니다.
    *
    * @details
    * SourceActor에게 Effect를 사용하여 SkillPoint를 특정 포인트로 변환합니다.
    * DamagePoint, HealPoint 등
    * 
    * @return true를 반환 시 EffectHandle이 생성되었습니다.
    * @return false를 반환 시 EffectHandle이 생성되지 않았습니다.
    */
    virtual bool ApplyOtherPointToSkillPoint(
        float SkillPoint,
        TWeakObjectPtr<class UBoardActorModel> SourceActor,
        OUT struct FActiveTacticalEffectHandle& EffectHandle) PURE_VIRTUAL(UStaticSkillEffect_Base::ApplyOtherPointToSkillPoint, return false;);
    

    /**
    * @brief 효과를 적용합니다.
    *
    * @details
    * 
    * @note 추후 Context와 같은 구조체로 전달하도록 변경이 필요해보입니다.
    */
    virtual void ApplySkillEffect(
        float SkillPoint,
        TWeakObjectPtr<class UBoardActorModel> SourceActor,
        TWeakObjectPtr<class UBoardActorModel> TargetActor,
        struct FBoardCombatTargetSnapshotData* SourceSnapShot = nullptr, 
        struct FBoardCombatTargetSnapshotData* TargetSnapShot = nullptr) PURE_VIRTUAL(UStaticSkillEffect_Base::ApplySkillEffect, return;);

};


