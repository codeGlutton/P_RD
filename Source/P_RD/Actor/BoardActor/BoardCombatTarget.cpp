#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"

bool IBoardCombatTarget::IsTargetable() const
{
	return IsDead() == false;
}

bool IBoardCombatTarget::IsDead() const
{
	return GetAttributeComponentModel()->HasMatchingGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);
}

UBoardCombatTargetSnapshotData* IBoardCombatTarget::MakeSnapshotData() const
{
	UBoardCombatTargetSnapshotData* Snapshot = NewObject<UBoardCombatTargetSnapshotData>(GetAttributeComponentModel());
	GetAttributeComponentModel()->CaptureAllStates(OUT Snapshot);

	return Snapshot;
}

ETeamAttitude::Type IBoardCombatTarget::GetTeamAttitudeTowards(const UObject& Other) const
{
	const IBoardCombatTarget* BoardCombatTarget = Cast<const IBoardCombatTarget>(&Other);
	if (BoardCombatTarget != nullptr)
	{
		return FGenericTeamId::GetAttitude(GetGenericTeamId(), BoardCombatTarget->GetGenericTeamId());
	}
	return ETeamAttitude::Neutral;
}

