#include "TAS/Effect/Tag/TacticalEffect_ActorState.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Simulation/Logger/EventLogger.h"

UTacticalEffect_ActorState::UTacticalEffect_ActorState()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
	mStackingType = ETacticalEffectStackingType::AggregateByTarget;
}

UTacticalEffect_AddActorState::UTacticalEffect_AddActorState()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_AddActorState::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	const int32 TagCount = FMath::FloorToInt(TESpec.mDynamicMagnitude);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance == nullptr)
	{
		return;
	}
	TSharedPtr<FTacticalEffectSpec> NewSpec = MakeShared<FTacticalEffectSpec>(mActorStateEffect.GetDefaultObject(), TESpec.GetContext());
	NewSpec->SetStackCount(TagCount);
	AttributeSetCompModelInstance->ApplyTacticalEffectSpecToSelf(*NewSpec);

	/* 로그 작성 */

	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();
	const FGameplayTagContainer& StateTags = GetDefault<UTacticalEffect>(mActorStateEffect)->GetAssetTags();
	for (const FGameplayTag& StateTag : StateTags)
	{
		FSRPGTagEffectEventLog Log;
		Log.mEffectTag = StateTag;
		Log.mCount = TagCount;

		GetWorldEventLogger(Instigator)->LogTagEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
	}
}

bool UTacticalEffect_GetActorState::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	if (mActorStateEffect == nullptr)
	{
		return false;
	}

	if (TESpec.mDynamicMagnitude <= 0.f)
	{
		return false;
	}

	return true;
}
