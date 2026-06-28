#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"
#include "AttributeSet/UnitAttributeSet.h"

UTacticalEffect_Stat_Damage::UTacticalEffect_Stat_Damage()
{
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;

	FTacticalModifierInfo Info;
	Info.mAttribute = UUnitAttributeSet::GetHPAttribute();
	Info.mModifierOp = EGameplayModOp::Additive;
	Info.mModifierMagnitude = -1.f;

	mModifiers.Add(Info);
}
