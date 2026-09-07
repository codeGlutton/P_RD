#include "TAS/Effect/Spawn/TacticalEffect_SpawnBoardActor.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "TAS/Effect/TacticalEffectContext.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

#include "Simulation/Logger/EventLogger.h"

UTacticalEffect_SpawnBoardActor::UTacticalEffect_SpawnBoardActor()
{
	mDurationPolicy = ETacticalEffectDurationType::Instant;
	mStackingType = ETacticalEffectStackingType::None;
}

void UTacticalEffect_SpawnBoardActor::OnExecuted(FActiveTacticalEffectsContainer& ActiveTEContainer, FTacticalEffectSpec& TESpec) const
{
	Super::OnExecuted(ActiveTEContainer, TESpec);

	const UBoardCombatTargetSnapshotData* TargetData = TESpec.GetTargetSnapshotData();
	if (TargetData == nullptr)
	{
		return;
	}
	
	// if ()
}

