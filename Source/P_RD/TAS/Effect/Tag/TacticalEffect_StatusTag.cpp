#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Simulation/Logger/EventLogger.h"

UTacticalEffect_StatusTag::UTacticalEffect_StatusTag()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_StatusTag::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	if (mStatusTag.IsValid() == false)
	{
		return;
	}

	const int32 TagCount = FMath::FloorToInt(TESpec.mDynamicMagnitude);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance == nullptr)
	{
		return;
	}

	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();
	AttributeSetCompModelInstance->AddLooseGameplayTag(mStatusTag, TagCount);

	/* 로그 작성 */

	FSRPGTagEffectEventLog Log;
	Log.mEffectTag = mStatusTag;
	Log.mCount = TagCount;

	GetWorldEventLogger(Instigator)->LogTagEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
}

bool UTacticalEffect_GetStatusTag::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	if (mStatusTag.IsValid() == false)
	{
		return false;
	}

	if (TESpec.mDynamicMagnitude <= 0.f)
	{
		return false;
	}

	return true;
}
