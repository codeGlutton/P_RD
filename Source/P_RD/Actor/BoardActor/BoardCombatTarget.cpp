#include "Actor/BoardActor/BoardCombatTarget.h"

bool IBoardCombatTarget::IsTargetable() const
{
	return true;
}

bool IBoardCombatTarget::IsDead() const
{
	//return GetAbilitySystemComponent()->HasMatchingGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);

	return false;
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

