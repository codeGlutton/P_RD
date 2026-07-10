#include "TAS/Effect/Tag/TacticalEffect_Dead.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "Simulation/Logger/EventLogger.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UTacticalEffect_Dead::UTacticalEffect_Dead()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_Dead::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (IsValid(AttributeSetCompModelInstance) == false)
	{
		return;
	}
	const UActorModel* Target = AttributeSetCompModelInstance->GetOwnerModel();

	AttributeSetCompModelInstance->AddLooseGameplayTag(EffectTags::GameplayEffect_ActorState_Dead, 1);

	FSRPGTagEffectEventLog Log;
	Log.mEffectTag = EffectTags::GameplayEffect_ActorState_Dead;
	Log.mCount = 1;

	if (IsValid(Target))
	{
		if (UEventLogger* EventLogger = GetWorldEventLogger(Target))
		{
			EventLogger->LogTagEffect(Target->GetModelId(), Target->GetClass(), Log);
		}
	}
}
