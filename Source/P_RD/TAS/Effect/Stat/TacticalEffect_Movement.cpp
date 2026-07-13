/*****************************************************************//**
 * @file   TacticalEffect_Movement.cpp
 * @brief  Movement 이펙트 구현
 * @author 이문환
 * @date   2026-07-01
 *********************************************************************/

#include "TAS/Effect/Stat/TacticalEffect_Movement.h"
#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"

UTacticalEffect_Movement::UTacticalEffect_Movement()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UCombatTargetAttributeSet::GetMovementAttribute();
	Info.mModifierOp = ETacticalModOp::AddBase;
	Info.mModifierMagnitude = 1.f;

	mModifiers.Add(Info);
}

void UTacticalEffect_Movement::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	FSRPGAttributeEffectEventLog Log;
	Log.mEffectAttribute = UCombatTargetAttributeSet::GetMovementAttribute();
	Log.mMagnitude = TESpec.mModifierValues[0];

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();

	GetWorldEventLogger(Instigator)->LogAttributeEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
}
