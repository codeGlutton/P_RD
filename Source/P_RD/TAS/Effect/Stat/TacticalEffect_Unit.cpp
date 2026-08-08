#include "TAS/Effect/Stat/TacticalEffect_Unit.h"

#include "AttributeSet/UnitAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "TAS/Effect/ActiveTacticalEffectsContainer.h"

bool UTacticalEffect_Unit::CanApply(const FActiveTacticalEffectsContainer& ActiveTEContainer, const FTacticalEffectSpec& TESpec) const
{
	if (Super::CanApply(ActiveTEContainer, TESpec) == false)
	{
		return false;
	}

	const UAttributeSetComponentModel* AttributeSetComponentModel = ActiveTEContainer.mOwner.Get();
	if (AttributeSetComponentModel == nullptr)
	{
		return false;
	}

	if (AttributeSetComponentModel->GetAttributeSet<UUnitAttributeSet>() == nullptr)
	{
		return false;
	}

	return true;
}
