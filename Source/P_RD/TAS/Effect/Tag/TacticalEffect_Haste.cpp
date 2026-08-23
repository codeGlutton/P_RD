// Fill out your copyright notice in the Description page of Project Settings.


#include "TAS/Effect/Tag/TacticalEffect_Haste.h"

UTacticalEffect_Haste::UTacticalEffect_Haste()
{
	mCachedAssetTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Haste);
	mCachedGrantedTags.AddTag(EffectTags::GameplayEffect_StatusEffect_RoundDuration_Buff_Haste);
}

UTacticalEffect_AddHaste::UTacticalEffect_AddHaste()
{
	mStatusEffect = UTacticalEffect_Haste::StaticClass();
}

UTacticalEffect_GetHaste::UTacticalEffect_GetHaste()
{
	mStatusEffect = UTacticalEffect_Haste::StaticClass();
}
