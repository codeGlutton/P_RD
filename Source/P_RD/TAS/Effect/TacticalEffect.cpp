#include "TAS/Effect/TacticalEffect.h"

FTacticalEffectSpec::FTacticalEffectSpec(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext) :
	FTacticalEffectSpec()
{
	Initialize(Class, EffectContext);
}

FTacticalEffectSpec::FTacticalEffectSpec(const FTacticalEffectSpec& Other, UTacticalEffectContext* EffectContext)
{
	*this = Other;
	mEffectContext = EffectContext;
}

void FTacticalEffectSpec::Initialize(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext)
{
	mEffectClass = Class;
	check(mEffectClass != nullptr);

	mIsInfinite = mEffectClass->mDurationPolicy == ETacticalEffectDurationType::Infinite;

	SetContext(EffectContext);
	mModifierValues.SetNum(mEffectClass->mModifiers.Num());
}

void FTacticalEffectSpec::SetStackCount(int32 NewStackCount)
{
	mStackCount = NewStackCount;
}

void FTacticalEffectSpec::SetContext(UTacticalEffectContext* NewEffectContext)
{
	mEffectContext = NewEffectContext;
}

int32 FTacticalEffectSpec::GetStackCount() const
{
	return mStackCount;
}

UTacticalEffectContext* FTacticalEffectSpec::GetContext() const
{
	return mEffectContext;
}

void FTacticalEffectSpec::CalculateModifierMagnitudes()
{
	for (int32 ModIdx = 0; ModIdx < mModifierValues.Num(); ++ModIdx)
	{
		const FTacticalModifierInfo& ModDef = mEffectClass->mModifiers[ModIdx];
		float& ModifierValue = mModifierValues[ModIdx];

		ModifierValue = ModDef.mModifierMagnitude * mDynamicMagnitude;
	}
}

float FTacticalEffectSpec::GetModifierMagnitude(int32 ModifierIdx) const
{
	check(mModifierValues.IsValidIndex(ModifierIdx) && mEffectClass != nullptr && mEffectClass->mModifiers.IsValidIndex(ModifierIdx));

	const float SingleEvaluatedMagnitude = mModifierValues[ModifierIdx];
	float ModMagnitude = SingleEvaluatedMagnitude;
	if (mEffectClass->mFactorInStackCount == true)
	{
		/* 스택 반영 */

		ModMagnitude = GameplayEffectUtilities::ComputeStackedModifierMagnitude(SingleEvaluatedMagnitude, GetStackCount(), mEffectClass->mModifiers[ModifierIdx].mModifierOp);
	}
	return ModMagnitude;
}

bool FTacticalModifierInfo::operator==(const FTacticalModifierInfo& Other) const
{
	if (mAttribute != Other.mAttribute)
	{
		return false;
	}

	if (mModifierOp != Other.mModifierOp)
	{
		return false;
	}

	return true;
}

bool FTacticalModifierInfo::operator!=(const FTacticalModifierInfo& Other) const
{
	return !(*this == Other);
}

bool UTacticalEffect::CanApply(const FActiveTacticalEffectsContainer& ActiveGEContainer, const FTacticalEffectSpec& GESpec) const
{
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent && !GEComponent->CanGameplayEffectApply(ActiveGEContainer, GESpec))
		{
			return false;
		}
	}*/

	return true;
}

bool UTacticalEffect::OnAddedToActiveContainer(FActiveTacticalEffectsContainer& ActiveGEContainer, FActiveTacticalEffect& ActiveGE) const
{
	bool ShouldBeActive = true;
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			bShouldBeActive = GEComponent->OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE) && bShouldBeActive;
		}
	}*/

	return ShouldBeActive;
}

void UTacticalEffect::OnExecuted(FActiveTacticalEffectsContainer& ActiveGEContainer, FTacticalEffectSpec& GESpec) const
{
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			GEComponent->OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
		}
	}*/
}

void UTacticalEffect::OnApplied(FActiveTacticalEffectsContainer& ActiveGEContainer, FTacticalEffectSpec& GESpec) const
{
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			GEComponent->OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);
		}
	}*/
}
