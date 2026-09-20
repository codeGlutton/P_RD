#include "TAS/Effect/TacticalEffect.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "TAS/Effect/TacticalEffectContext.h"

FTacticalEffectSpec::FTacticalEffectSpec(const UTacticalEffect* Class, UTacticalEffectContext* EffectContext) : FTacticalEffectSpec()
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

	SetContext(EffectContext);

	// CDO의 모디파이어 개수만큼 평가 결과 값 버퍼를 미리 확보
	mModifiers.SetNum(mEffectClass->mModifiers.Num());
}

bool FTacticalEffectSpec::AttemptCalculateDurationFromDef(OUT int32& ResultDuration) const
{
	check(mEffectClass != nullptr);

	bool IsCalculatedDuration = true;

	const ETacticalEffectDurationType DurType = mEffectClass->mDurationPolicy;
	if (DurType == ETacticalEffectDurationType::Infinite)
	{
		ResultDuration = FTacticalEffectConstants::INFINITE_DURATION;
	}
	else if (DurType == ETacticalEffectDurationType::Instant)
	{
		ResultDuration = FTacticalEffectConstants::NO_DURATION;
	}
	else
	{
		const int32 FinalDuration = mEffectClass->mDurationMagnitude * mDynamicDurationMagnitude;
		if (FinalDuration > 0)
		{
			ResultDuration = FinalDuration;
		}
		else
		{
			IsCalculatedDuration = false;
		}
	}

	return IsCalculatedDuration;
}

void FTacticalEffectSpec::SetDuration(int32 NewDuration)
{
	mDuration = NewDuration;
}

void FTacticalEffectSpec::SetStackCount(int32 NewStackCount)
{
	mStackCount = NewStackCount;
}

void FTacticalEffectSpec::SetContext(UTacticalEffectContext* NewEffectContext)
{
	mEffectContext = NewEffectContext;
}

void FTacticalEffectSpec::SetInstigatorSnapshotData(UBoardCombatTargetSnapshotData* SnapshotData)
{
	mInstigatorSnapshotData = SnapshotData;
}

void FTacticalEffectSpec::SetTargetSnapshotData(UBoardCombatTargetSnapshotData* SnapshotData)
{
	mTargetSnapshotData = SnapshotData;
}

int32 FTacticalEffectSpec::GetDuration() const
{
	return mDuration;
}

int32 FTacticalEffectSpec::GetStackCount() const
{
	return mStackCount;
}

UTacticalEffectContext* FTacticalEffectSpec::GetContext() const
{
	return mEffectContext;
}

UBoardCombatTargetSnapshotData* FTacticalEffectSpec::GetInstigatorSnapshotData() const
{
	return mInstigatorSnapshotData;
}
UBoardCombatTargetSnapshotData* FTacticalEffectSpec::GetTargetSnapshotData() const
{
	return mTargetSnapshotData;
}

bool FTacticalEffectSpec::IsValidDuration() const
{
	if (mEffectClass->mDurationPolicy == ETacticalEffectDurationType::Duration)
	{
		if (mDynamicDurationMagnitude == 0 || mEffectClass->mDurationMagnitude == 0)
		{
			return false;
		}
	}
	return true;
}

void FTacticalEffectSpec::CalculateModifierMagnitudes()
{
	for (int32 ModIdx = 0; ModIdx < mModifiers.Num(); ++ModIdx)
	{
		const FTacticalModifierInfo& ModDef = mEffectClass->mModifiers[ModIdx];
		float& EvaluatedMagnitude = mModifiers[ModIdx];

		// 정적 크기 x 동적 배율 = 스택 1개 기준 단일 모디파이어 크기.
		EvaluatedMagnitude = ModDef.mModifierMagnitude * mDynamicMagnitude;
	}
}

float FTacticalEffectSpec::GetStackedModifierMagnitude(int32 ModifierIdx) const
{
	check(mModifiers.IsValidIndex(ModifierIdx) && mEffectClass != nullptr && mEffectClass->mModifiers.IsValidIndex(ModifierIdx));

	// CalculateModifierMagnitudes 가 캐시한 스택 1개 기준 단일 크기.
	const float SingleEvaluatedMagnitude = mModifiers[ModifierIdx];
	float ModMagnitude = SingleEvaluatedMagnitude;
	if (mEffectClass->mFactorInStackCount == true)
	{
		/* 스택 반영 */

		ModMagnitude = TacticalEffectUtilities::ComputeStackedModifierMagnitude(SingleEvaluatedMagnitude, GetStackCount(), mEffectClass->mModifiers[ModifierIdx].mModifierOp);
	}
	return ModMagnitude;
}

const FTacticalEffectModifiedAttribute* FTacticalEffectSpec::GetModifiedAttribute(const FTacticalAttribute& Attribute) const
{
	for (const FTacticalEffectModifiedAttribute& ModifiedAttribute : mModifiedAttributes)
	{
		if (ModifiedAttribute.mAttribute == Attribute)
		{
			return &ModifiedAttribute;
		}
	}
	return nullptr;
}

FTacticalEffectModifiedAttribute* FTacticalEffectSpec::GetModifiedAttribute(const FTacticalAttribute& Attribute)
{
	for (FTacticalEffectModifiedAttribute& ModifiedAttribute : mModifiedAttributes)
	{
		if (ModifiedAttribute.mAttribute == Attribute)
		{
			return &ModifiedAttribute;
		}
	}
	return nullptr;
}

FTacticalEffectModifiedAttribute* FTacticalEffectSpec::AddModifiedAttribute(const FTacticalAttribute& Attribute)
{
	FTacticalEffectModifiedAttribute NewAttribute;
	NewAttribute.mAttribute = Attribute;
	return &mModifiedAttributes[mModifiedAttributes.Add(NewAttribute)];
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

bool UTacticalEffect::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	// GameplayEffectComponent 기반 적용 검사 
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent && !GEComponent->CanGameplayEffectApply(ActiveTEContainer, TESpec))
		{
			return false;
		}
	}*/

	return true;
}

bool UTacticalEffect::OnAddedToActiveContainer(FActiveTacticalEffectsContainer& ActiveTEContainer, FActiveTacticalEffect& ActiveTE) const
{
	bool ShouldBeActive = true;
	// GameplayEffectComponent 추가 콜백
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			bShouldBeActive = GEComponent->OnActiveGameplayEffectAdded(ActiveTEContainer, ActiveTE) && bShouldBeActive;
		}
	}*/

	return ShouldBeActive;
}

void UTacticalEffect::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	// GameplayEffectComponent 실행 콜백
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			GEComponent->OnGameplayEffectExecuted(ActiveTEContainer, TESpec);
		}
	}*/
}

void UTacticalEffect::OnApplied(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	// GameplayEffectComponent 적용 콜백
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			GEComponent->OnGameplayEffectApplied(ActiveTEContainer, TESpec);
		}
	}*/
}

void UTacticalEffect::OnReduceTimeRemaining(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	// GameplayEffectComponent 적용 콜백
	// - GEComponent도 구현해둘까 고민중 by Mohojae
	/*for (const UGameplayEffectComponent* GEComponent : GEComponents)
	{
		if (GEComponent)
		{
			GEComponent->OnGameplayEffectReduceTimeRemaining(ActiveTEContainer, TESpec);
		}
	}*/
}
