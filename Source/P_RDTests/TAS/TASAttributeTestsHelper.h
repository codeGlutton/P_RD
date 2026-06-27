/*****************************************************************//**
 * @file   TASAttributeTestsHelper.h
 * @brief  TAS Attribute 테스트를 위한 Mock 모델 정의 헤더
 * @author 모호재
 * @date   2026-06-26
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Actor/ActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "Singleton/WorldSubsystem/SimulationSubsystem.h"
#include "TASAttributeTestsHelper.generated.h"

/**
 * @brief USimulationSubsystem의 protected 멤버 접근용 우회 클래스
 */
class FSimulationSubsystemTestAccessor : public USimulationSubsystem
{
public:
    static URoomInstance* GetRoomInstance(USimulationSubsystem* Subsystem)
    {
        if (Subsystem == nullptr) 
        {
            return nullptr;
        }

        const FSimulationSubsystemTestAccessor* Accessor = static_cast<const FSimulationSubsystemTestAccessor*>(Subsystem);
        return Accessor->mCurrentRoomContext->mRoomInstance;
    }

    static void SetSimState(USimulationSubsystem* Subsystem, ESRPGSimulationState State)
    {
        if (Subsystem == nullptr) 
        {
            return;
        }

        FSimulationSubsystemTestAccessor* Accessor = static_cast<FSimulationSubsystemTestAccessor*>(Subsystem);
        Accessor->SetSimulationState(State);
    }
};

UCLASS()
class UTASActorModelMock : public UActorModel
{
	GENERATED_BODY()

public:
	UTASActorModelMock()
	{
		mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
		mUnitAttributeSet = CreateDefaultSubobject<UUnitAttributeSet>(TEXT("UnitAttributeSet"));
	}

public:
	virtual void PreInitializeComponentModels() override {}
	virtual void PostInitializeComponentModels() override {}

public:
	UAttributeSetComponentModel* GetAttributeComponent() const
	{
		return mAttributeCompModel;
	}

private:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "UnitAttributeSet"))
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;

	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	TObjectPtr<UAttributeSetComponentModel> mAttributeCompModel;
};

UCLASS()
class UTestInstantTacticalEffect : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTestInstantTacticalEffect()
	{
		mDurationPolicy = ETacticalEffectDurationType::Instant;
		mStackingType = ETacticalEffectStackingType::None;

		FTacticalModifierInfo Info;
		Info.mAttribute = UUnitAttributeSet::GetMaxHPAttribute();
		Info.mModifierOp = EGameplayModOp::Additive;
		Info.mModifierMagnitude = 1.f;

		mModifiers.Add(Info);
	}
};

UCLASS()
class UTestInfiniteTacticalEffect : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTestInfiniteTacticalEffect()
	{
		mDurationPolicy = ETacticalEffectDurationType::Infinite;
		mStackingType = ETacticalEffectStackingType::None;

		FTacticalModifierInfo Info;
		Info.mAttribute = UUnitAttributeSet::GetDefensePointAttribute();
		Info.mModifierOp = EGameplayModOp::Additive;
		Info.mModifierMagnitude = 1.f;

		mModifiers.Add(Info);
	}
};

UCLASS()
class UTestInfiniteTagEffect : public UTacticalEffect
{
	GENERATED_BODY()

public:
	UTestInfiniteTagEffect()
	{
		mDurationPolicy = ETacticalEffectDurationType::Infinite;
		mStackingType = ETacticalEffectStackingType::None;

		mCachedGrantedTags.AddTag(AbilityTags::GameplayAbility_Passive_OnEndAttacking);
	}
};