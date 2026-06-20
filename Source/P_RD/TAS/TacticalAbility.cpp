// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/TacticalAbility.h"

#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"

void UTacticalAbility::ApplyEffect(const FTacticalAbilityContext& Context, TArray<class UTacticalEffectContext*>& EffectContext)
{
	// 각각의 타일에게 효과를 적용한다.
	for (int32 i = 0; i < Context.mTargetTile.Num(); ++i)
	{
		NewObject<UTacticalEffect_Stat_Damage>()->ActivateEffect(*Context.mCasterActor.Get(), Context.mTargetTile[i], EffectContext);
	}
}
