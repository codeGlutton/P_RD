#include "UI/Combat/CombatUIDebugFixture.h"

#include "GameplayTagType.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	int32 GCombatUIFixtureMode = 0;
	FAutoConsoleVariableRef CCombatUIFixtureMode(
		TEXT("rd.Debug.CombatUIFixture"), GCombatUIFixtureMode,
		TEXT("Combat UI fixture: 0=off, 1=display-only HP one, 2=buff/debuff/both statuses, 3=all."));

	int32 GForceCombatHPOne = 0;
	FAutoConsoleVariableRef CForceCombatHPOne(
		TEXT("rd.Debug.ForceCombatHPOne"), GForceCombatHPOne,
		TEXT("Explicit opt-in: 1 mutates enemy combat units' actual HP to 1. "
			"The -EnemyHPOne command-line switch enables the same fixture."));
}

int32 CombatUIDebugFixture::GetMode()
{
#if UE_BUILD_SHIPPING
	return 0;
#else
	return GCombatUIFixtureMode;
#endif
}

bool CombatUIDebugFixture::ShouldMutateActualHPOne()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	// Deliberately independent from CombatUIFixture mode. The normal UI fixture
	// must not alter combat outcome or the HP tracked by RunPersistData.
	return GForceCombatHPOne != 0
		|| FParse::Param(FCommandLine::Get(), TEXT("EnemyHPOne"));
#endif
}

void CombatUIDebugFixture::ApplyDisplayHPOne(const bool bUnitAlive,
	OUT FUnitUI& UnitUIData)
{
#if UE_BUILD_SHIPPING
	return;
#else
	if (bUnitAlive && (GCombatUIFixtureMode == 1 || GCombatUIFixtureMode == 3))
	{
		UnitUIData.mHP = 1.f;
	}
#endif
}

void CombatUIDebugFixture::AppendStatuses(const bool bPlayer,
	const int32 SideIndex, OUT TArray<FStatusEffectUI>& Statuses)
{
#if UE_BUILD_SHIPPING
	return;
#else
	if (GCombatUIFixtureMode != 2 && GCombatUIFixtureMode != 3)
	{
		return;
	}

	const FGameplayTag Buff = bPlayer
		? EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification
		: EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Vigor;
	const FGameplayTag Debuff = bPlayer
		? EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness
		: EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability;
	const bool bAddBuff = SideIndex == 0 || SideIndex == 2;
	const bool bAddDebuff = SideIndex == 1 || SideIndex == 2;
	const auto AddIfMissing = [&Statuses](const FGameplayTag Tag, const int32 Stack)
	{
		if (Statuses.ContainsByPredicate([Tag](const FStatusEffectUI& Existing)
			{ return Existing.mTag == Tag; }) == false)
		{
			FStatusEffectUI& Added = Statuses.AddDefaulted_GetRef();
			Added.mTag = Tag;
			Added.mStackCount = Stack;
		}
	};
	if (bAddBuff)
	{
		AddIfMissing(Buff, 1);
	}
	if (bAddDebuff)
	{
		AddIfMissing(Debuff, 2);
	}
#endif
}
