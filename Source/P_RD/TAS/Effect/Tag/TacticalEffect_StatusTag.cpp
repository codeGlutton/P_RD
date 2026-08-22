#include "TAS/Effect/Tag/TacticalEffect_StatusTag.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Simulation/Logger/EventLogger.h"

UTacticalEffect_Status::UTacticalEffect_Status()
{
	mStackingType = ETacticalEffectStackingType::AggregateByTarget;
}

UTacticalEffect_InfiniteStatus::UTacticalEffect_InfiniteStatus()
{
	mDurationPolicy = ETacticalEffectDurationType::Infinite;
}

UTacticalEffect_DurationStatus::UTacticalEffect_DurationStatus()
{
	mDurationPolicy = ETacticalEffectDurationType::Duration;
	mDurationUnitPolicy = ETacticalEffectDurationUnitType::EveryRound;
	mDurationMagnitude = 1;

	mDurationUnitPolicy = ETacticalEffectDurationUnitType::EveryRound;
	mStackDurationRefreshPolicy = ETacticalEffectStackingDurationPolicy::NeverRefresh;
	mStackExpirationPolicy = ETacticalEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
}

UTacticalEffect_AddStatus::UTacticalEffect_AddStatus()
{
	// 즉시형
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_AddStatus::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	const int32 TagCount = FMath::FloorToInt(TESpec.mDynamicMagnitude);

	UAttributeSetComponentModel* AttributeSetCompModelInstance = ActiveTEContainer.mOwner.Get();
	if (AttributeSetCompModelInstance == nullptr)
	{
		return;
	}
	TSharedPtr<FTacticalEffectSpec> NewSpec = MakeShared<FTacticalEffectSpec>(mStatusEffect.GetDefaultObject(), TESpec.GetContext());
	NewSpec->SetStackCount(TagCount);
	AttributeSetCompModelInstance->ApplyTacticalEffectSpecToSelf(*NewSpec);

	/* 로그 작성 */

	const UActorModel* Instigator = AttributeSetCompModelInstance->GetOwnerModel();
	const FGameplayTagContainer& StatusTags = GetDefault<UTacticalEffect>(mStatusEffect)->GetAssetTags();
	for (const FGameplayTag& StatusTag : StatusTags)
	{
		FSRPGTagEffectEventLog Log;
		Log.mEffectTag = StatusTag;
		Log.mCount = TagCount;

		GetWorldEventLogger(Instigator)->LogTagEffect(Instigator->GetModelId(), Instigator->GetClass(), Log);
	}
}

bool UTacticalEffect_GetStatus::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	if (mStatusEffect == nullptr)
	{
		return false;
	}

	if (TESpec.mDynamicMagnitude <= 0.f)
	{
		return false;
	}

	return true;
}

