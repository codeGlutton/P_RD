#include "Actor/BoardActor/BoardCombatTarget.h"

UScriptStruct* FTileTargetSnapshotTargetData::GetScriptStruct() const
{
	return FTileTargetSnapshotTargetData::StaticStruct();
}

bool FTileTargetSnapshotTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& OutSuccess)
{
	Ar << mMaxHP;
	Ar << mHP;
	Ar << mSkillPoint;
	Ar << mDamagePoint;
	Ar << mDefensePoint;
	Ar << mMovementPoint;
	Ar << mBuffState;
	Ar << mDebuffState;

	return true;
}

bool IBoardCombatTarget::IsDead() const
{
	return GetAbilitySystemComponent()->HasMatchingGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);
}

bool IBoardCombatTarget::IsTargetable() const
{
	return true;
}
