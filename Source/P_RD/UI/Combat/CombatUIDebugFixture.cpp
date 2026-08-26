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

	// Development 테스트 패키지는 모든 전투 유닛을 한 대에 처치할 수 있게
	// 기본 활성화한다. Shipping에서는 아래 함수가 항상 false라 영향이 없다.
	int32 GForceCombatHPOne = 1;
	FAutoConsoleVariableRef CForceCombatHPOne(
		TEXT("rd.Debug.ForceCombatHPOne"), GForceCombatHPOne,
		TEXT("Development fixture: 1 mutates every combat unit's current HP to 1. "
			"Set 0 to disable. -AllHPOne/-EnemyHPOne also enables it."));
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
		|| FParse::Param(FCommandLine::Get(), TEXT("AllHPOne"))
		|| FParse::Param(FCommandLine::Get(), TEXT("EnemyHPOne"));
#endif
}

bool CombatUIDebugFixture::ShouldMutatePlayerHPOne()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	// 아군까지 건드리는 것은 명시적으로 2를 넣었을 때만이다. -EnemyHPOne은
	// 이름 그대로 적만 바꾼다.
	return GForceCombatHPOne >= 2;
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
		? EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Fortification
		: EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Vigor;
	const FGameplayTag Debuff = bPlayer
		? EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Weakness
		: EffectTags::GameplayEffect_StatusEffect_RoundDuration_Debuff_Vulnerability;
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
