// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/TacticalAbility.h"

#include "TAS/Effect/TacticalEffectContext.h"
#include "TAS/Effect/Stat/TacticalEffect_Stat_Damage.h"

void UTacticalAbility::ActivateAbility(const FTacticalAbilityContext Context)
{
	if (OnStartAbility.IsBound())
		OnStartAbility.Broadcast(Context.mCasterActor);
}

void UTacticalAbility::ApplyEffect(const FTacticalAbilityContext& Context, TArray<class UTacticalEffectContext*>& EffectContext)
{
	if (OnApplyAbility.IsBound())
		OnApplyAbility.Broadcast(Context.mCasterActor);

	// 각각의 타일에게 효과를 적용한다.
	for (int32 i = 0; i < Context.mTargetTile.Num(); ++i)
	{
		for(int32 j = 0; j < EffectContext.Num(); ++j)
		{
			UTacticalEffect* EffectCDO = EffectContext[j]->mTacticalEffect->GetDefaultObject<UTacticalEffect>();
			EffectCDO->ActivateEffect(*Context.mCasterActor.Get(), Context.mTargetTile[i], EffectContext[j]);
		}

	}
}

void UTacticalAbility::EndAbility(const FTacticalAbilityContext& Context)
{
	if (OnEndAbility.IsBound())
		OnEndAbility.Broadcast(Context.mCasterActor);
}