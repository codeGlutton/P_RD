// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/TacticalAbility.h"

void UTacticalAbility::ApplyEffect(const FTacticalAbilityContext& Context, const TArray<class UTacticalEffectContext*>& EffectContext)
{
	// 각각의 타일에게 효과를 적용한다.
	for (int32 i = 0; i < Context.mTargetTile.Num(); ++i)
	{
		// 타일에게 효과를 적용한다.
		
	}
}
